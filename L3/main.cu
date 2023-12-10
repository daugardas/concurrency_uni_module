#include <iostream>
#include "json.hpp"
#include <vector>
#include <fstream>
#include <cuda.h>
#include <cuda_runtime.h>
#include <stdio.h>

constexpr int RESULT_LENGTH = 366;
constexpr int MAX_NAME_LENGTH = 20;
constexpr int HASHED_STRING_LENGHT = 30;

using json = nlohmann::json;
using namespace std;

struct Person
{
    int id;
    double age;
    char name[MAX_NAME_LENGTH];
};

vector<Person> load_json_file(const string &file_name)
{
    ifstream f(file_name);
    if (!f.is_open())
    {
        cerr << "Failed to open given '" << file_name << "' file." << endl;
        return std::vector<Person>();
    }
    json data = json::parse(f);
    vector<Person> data_vector;
    for (auto &element : data)
    {
        Person person;
        element.at("id").get_to(person.id);
        element.at("age").get_to(person.age);
        string name_str = element.at("name");
        strncpy(person.name, name_str.c_str(), MAX_NAME_LENGTH);
        person.name[MAX_NAME_LENGTH - 1] = '\0';
        data_vector.push_back(person);
    }
    f.close();

    return data_vector;
}

__device__ void hash_person(const Person *person, unsigned char *hash)
{
    // printf("person name: %s, age: %f, id: %d\n", person->name, person->age, person->id);
    for (int i = 0; i < HASHED_STRING_LENGHT - 1; i++)
    {
        if (i < 15)
        {
            // the first 15 characters will be name hash
            int name = (int)pow((int)person->name % 25 * i, 2);
            char randomChar = (char)(name % 25 + 'A');
            hash[i] = randomChar;
        }
        else if (i > 15 && i < 23)
        {
            // 16th-23rd characters will be id hash
            char randomChar = (char)((int)pow(person->id * i, 2) % 25 + 'A');
            hash[i] = randomChar;
        }
        else if (i > 23)
        {
            // last characters will be age hash
            char randomChar = (char)((int)pow(person->age * i, 3) % 25 + 'A');
            hash[i] = randomChar;
        }
    }

    hash[HASHED_STRING_LENGHT - 1] = '\0';
}

__device__ bool hash_passes_test(unsigned char *hash)
{
    int frequency[256] = {0}; // Initialize all frequencies to 0

    for (int i = 0; i < HASHED_STRING_LENGHT; i++)
    {
        frequency[hash[i]]++; // Increment the frequency of the current character

        if (frequency[hash[i]] >= 6) // If the current character appears four times or more
        {
            return true;
        }
    }
    return false;
}

__global__ void calculate_hashes(const Person *persons, unsigned char *result, const int *persons_count, int *last_filled_person_index)
{
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x; // total number of threads

    // printf("Starting calculating. Index: %d, stride: %d\n", index, stride);

    if (index < RESULT_LENGTH)
    {
        // printf("Calculating hashes for index: %d\n", index);
        for (int i = index; i < *persons_count; i += stride)
        {
            unsigned char hashed_string[HASHED_STRING_LENGHT];
            memset(hashed_string, ' ', HASHED_STRING_LENGHT);
            hash_person(&persons[i], hashed_string);
            hashed_string[HASHED_STRING_LENGHT - 1] = '\n';

            bool passed = hash_passes_test(hashed_string);
            printf("Hashed string: %s, passed: %d\n", hashed_string, passed);
            if (passed)
            {
                const int current_index = atomicAdd(last_filled_person_index, 1);
                // printf("last filled index: %d\n", current_index);
                memcpy(result + HASHED_STRING_LENGHT * current_index, hashed_string, HASHED_STRING_LENGHT);
            }
            else
            {
                printf("Skipped.\n");
            }
        }
    }
}

int main()
{
    // read data
    const string data_file_name = "data.json";
    const vector<Person> data = load_json_file(data_file_name);
    cout << "Loaded " << data.size() << " persons from '" << data_file_name << "'." << std::endl;
    const bool data_exists = data.size() > 0;
    if (!data_exists)
    {
        cerr << "There is no data in '" + data_file_name + "'. Closing the program." << std::endl;
        return 1;
    }

    int device;
    cudaError_t error = cudaGetDevice(&device);
    if (error)
    {
        cerr << "Encountered an error while getting device number. Error: " << error << endl;
        return 1;
    }

    // unsigned char *result = new unsigned char[RESULT_LENGTH * HASHED_STRING_LENGHT];
    unsigned char *result;
    error = cudaMalloc(&result, RESULT_LENGTH * HASHED_STRING_LENGHT);
    if (error)
    {
        cerr << "Couldn't allocate memory for result on GPU. Error: " << error << endl;
        return 1;
    }

    cudaDeviceProp prop;
    error = cudaGetDeviceProperties(&prop, device);
    if (error)
    {
        cerr << "Error while getting CUDA " << device << " device properties. Error: " << error << endl;
        cudaFree(result);
        return 1;
    }

    // const int MAX_THREADS_IN_BLOCK = prop.maxThreadsPerBlock;
    // const int MAX_THREADS_IN_GPU = prop.maxThreadsPerMultiProcessor;
    // const int MAX_BLOCKS_IN_GPU = prop.maxBlocksPerMultiProcessor;
    //
    // cout << "Max threads in block: " << MAX_THREADS_IN_BLOCK << endl; // 1024
    // cout << "Max threads in gpu: " << MAX_THREADS_IN_GPU << endl;     // 1536
    // cout << "Max blocks in gpu: " << MAX_BLOCKS_IN_GPU << endl;       // 16

    const int THREADS_IN_BLOCK = 32;
    const int BLOCK_COUNT = 2;

    const int PERSONS_COUNT = data.size();
    const int PERSONS_SIZE = PERSONS_COUNT * sizeof(Person);

    Person *d_persons;
    error = cudaMalloc(&d_persons, PERSONS_SIZE);
    if (error)
    {
        cerr << "Couldn't allocate memory for persons on GPU. Error: " << error << endl;
        cudaFree(result);
        return 1;
    }

    error = cudaMemcpy(d_persons, data.data(), PERSONS_SIZE, cudaMemcpyHostToDevice);
    if (error)
    {
        cerr << "Couldn't copy persons to GPU. Error: " << error << endl;
        cudaFree(result);
        cudaFree(d_persons);
        return 1;
    }

    int *d_persons_count;
    error = cudaMalloc(&d_persons_count, sizeof(int));
    if (error)
    {
        cerr << "Couldn't allocate memory for persons count on GPU. Error: " << error << endl;
        cudaFree(result);
        cudaFree(d_persons);
        return 1;
    }

    error = cudaMemcpy(d_persons_count, &PERSONS_COUNT, sizeof(int), cudaMemcpyHostToDevice);
    if (error)
    {
        cerr << "Couldn't copy persons count to GPU. Error: " << error << endl;
        cudaFree(result);
        cudaFree(d_persons);
        cudaFree(d_persons_count);
        return 1;
    }

    int *last_filled_persons_index;
    cudaMalloc(&last_filled_persons_index, sizeof(int));
    cudaMemcpy(last_filled_persons_index, 0, sizeof(int), cudaMemcpyHostToDevice);
    calculate_hashes<<<BLOCK_COUNT, THREADS_IN_BLOCK>>>(d_persons, result, d_persons_count, last_filled_persons_index);

    error = cudaDeviceSynchronize();
    if (error)
    {
        cerr << "Couldn't synchronise CPU with GPU. Error: " << error << endl;
        cudaFree(result);
        cudaFree(d_persons);
        cudaFree(d_persons_count);
        return 1;
    }

    unsigned char *result_on_cpu = new unsigned char[RESULT_LENGTH * HASHED_STRING_LENGHT];
    error = cudaMemcpy(result_on_cpu, result, RESULT_LENGTH * HASHED_STRING_LENGHT, cudaMemcpyDeviceToHost);
    if (error)
    {
        cerr << "Couldn't copy result from GPU to CPU. Error: " << error << endl;
        cudaFree(result);
        cudaFree(d_persons);
        cudaFree(d_persons_count);
        delete[] result_on_cpu;
        return 1;
    }

    result_on_cpu[RESULT_LENGTH * HASHED_STRING_LENGHT - 1] = '\0';

    if (remove("results.txt") == 0)
    {
        printf("File deleted successfully\n");
    }
    else
    {
        printf("Unable to delete the file or file doesn't exist\n");
    }

    ofstream out("results.txt");
    out << result_on_cpu;
    out.close();

    printf("Results: \n");
    int results_length = strlen((char *)result_on_cpu);
    int person_i = 1;
    for (int i = 0; i < results_length; i++)
    {

        if (result_on_cpu[i] == '\n')
        {
            printf("\n%d: ", person_i);
            person_i++;
        }
        else if (result_on_cpu[i] == '\0')
            printf("NULL");
        else
            printf("%c", result_on_cpu[i]);
    }

    delete[] result_on_cpu;
    cudaFree(result);
    cudaFree(d_persons);
    cudaFree(d_persons_count);
    return 0;
}
