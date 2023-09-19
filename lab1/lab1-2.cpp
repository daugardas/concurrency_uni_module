#include <iostream>
#include <vector>
#include <fstream>
#include <random>
#include <omp.h>
#include <thread>
#include "json.hpp"
using json = nlohmann::json;

struct Person
{
	int id;
	double age;
	std::string name;
};

struct PersonWithChangedData
{
	Person originalData;
	int id;
	double age;
	std::string name;
};

class SortedResultMonitor
{
public:
	SortedResultMonitor(int size)
	{
		if (size < 1)
		{
			throw std::runtime_error("Incorrect initial given size to a SortedResultMonitor. Initial size has to be at least 1.");
		}

		persons = new PersonWithChangedData[size];
		this->size = size;
		this->size_used = 0;
	}
	~SortedResultMonitor()
	{
		delete[] persons;
	}
	void addItemSorted(PersonWithChangedData item)
	{
#pragma omp critical
		{
			std::cout << "Thread #" << omp_get_thread_num() << ": Adding item with age " << item.age << " to the sorted list.\n";
			// find the position to place the item, and if needed push other elements forwards
			if (size_used == 0)
			{
				persons[0] = item;
			}
			else
			{
				int index_to_insert = -1;
				for (int i = 0; i < size_used; i++)
					if (item.age < persons[i].age)
					{
						index_to_insert = i;
						break;
					}

				if (index_to_insert == -1)
					persons[size_used] = item;
				else
				{
					// shift the existing persons to the right
					for (int i = size_used; i > index_to_insert; i--)
						persons[i] = persons[i - 1];

					// insert the new person
					persons[index_to_insert] = item;
				}
			}
			size_used++;
		}
	}
	std::vector<PersonWithChangedData> getItems()
	{
		std::vector<PersonWithChangedData> items;
		for (int i = 0; i < size_used; i++)
		{
			items.push_back(persons[i]);
		}
		return items;
	}

private:
	PersonWithChangedData *persons;
	int size;
	int size_used;
};

void save_persons_table(const std::vector<Person> &data, const std::string &file_name, const std::string &title, const bool &append)
{
	std::cout << "saving " << data.size() << " persons.\n";

	std::ofstream o;
	if (append)
		o.open(file_name, std::ios_base::app);
	else
		o.open(file_name);

	if (data.size() > 0)
	{
		o << "_________________________________________________________" << std::endl;
		o << "| " << std::setw(52) << title << " |" << std::endl;
		o << "|-------------------------------------------------------|" << std::endl;
		o << "| ID          | Name                           | Age    |" << std::endl;
		o << "|-------------------------------------------------------|" << std::endl;
		for (auto &p : data)
		{
			o << "| " << std::setw(11) << p.id << " | " << std::setw(30) << p.name << " | " << std::setw(6) << p.age << " |" << std::endl;
		}
		o << "|-------------------------------------------------------|" << std::endl;
		o << std::endl;
	}
	else
	{
		o << "No people's data. Either there was no data to begin with, or all of it was filtered." << std::endl;
	}
	o.close();
}

void save_modified_persons_table(const std::vector<PersonWithChangedData> &data, const std::string &file_name, const std::string &title, const bool &append, const int &id_sum, const double &age_sum)
{
	std::cout << "saving " << data.size() << " modified persons.\n";

	std::ofstream o;
	if (append)
		o.open(file_name, std::ios_base::app);
	else
		o.open(file_name);

	if (data.size() > 0)
	{
		o << "_________________________________________________________" << std::endl;
		o << "| " << std::setw(52) << title << " |" << std::endl;
		o << "|-------------------------------------------------------|" << std::endl;
		o << "| ID          | Name                           | Age    |" << std::endl;
		o << "|-------------------------------------------------------|" << std::endl;
		for (auto &p : data)
		{
			o << "| " << std::setw(11) << p.id << " | " << std::setw(30) << p.name << " | " << std::setw(6) << p.age << " |" << std::endl;
		}
		o << "|-------------------------------------------------------|" << std::endl;
		o << "ID sum: " << id_sum << std::endl;
		o << "Age sum: " << age_sum << std::endl;
	}
	else
	{
		o << "No modified people's data. Either there was no data to begin with, or all of it was filtered." << std::endl;
	}
	o.close();
}

std::vector<Person> load_json_file(const std::string &file_name)
{
	std::ifstream f(file_name);
	if (!f.is_open())
	{
		std::cerr << "Failed to open given '" << file_name << "' file." << std::endl;
		return std::vector<Person>();
	}
	json data = json::parse(f);
	std::vector<Person> data_vector;
	for (auto &element : data)
	{
		Person person;
		element.at("id").get_to(person.id);
		element.at("age").get_to(person.age);
		element.at("name").get_to(person.name);
		data_vector.push_back(person);
	}
	f.close();

	return data_vector;
}

PersonWithChangedData modify_person_data(const Person &person)
{
	PersonWithChangedData p;
	p.originalData = person;

	// Generate a new id with very complex calculations
	p.id = 0;
	for (int i = 0; i < 1000000; i++)
	{
		p.id += person.id * i;
		for (int j = 0; j < 100; j++)
		{
			p.id += i * j;
		}
	}
	p.id /= 100000000;

	// Generate a new age with very complex calculations
	p.age = person.age;
	for (int i = 0; i < 1000000; i++)
	{
		if (p.age < 0)
			p.age += -p.age * 3.1425;
		else
			p.age += p.age * 3.1425;
		for (int j = 0; j < 100; j++)
		{
			p.age += i + j;
		}
		if (p.age < 0 || p.age > 10000)
		{
			p.age = person.age;
		}
	}

	// Generate a new name with very complex calculations
	p.name = "";
	for (int i = 0; i < 10; i++)
	{
		char randomChar = (char)((int)pow(person.id + i, 4) % 25 + 'A'); // Generate 'A' to 'Z'
		p.name += randomChar;
		for (int j = 0; j < 2; j++)
		{
			randomChar = (char)((int)pow(person.age + i * j, 5) % 25 + 'A'); // Generate 'A' to 'Z'
			p.name += randomChar;
		}
	}

	return p;
}

int main()
{
	std::string file_name = "filters_some.json";
	std::string results_file_name = "results_openmp.txt";
	std::vector<Person> data = load_json_file(file_name);
	std::cout << "Loaded " << data.size() << " persons from '" << file_name << "'." << std::endl;
	bool data_exists = data.size() > 0;
	if (!data_exists)
	{
		std::cerr << "There is no data in '" + file_name + "'. Closing the program." << std::endl;
		return 1;
	}

	SortedResultMonitor sorted_monitor(data.size());
	int full_id_sum = 0;
	double full_age_sum = 0;
#pragma omp parallel
	{
		int thread_id = omp_get_thread_num();
		// crude way of splitting the data between threads
		int num_threads = omp_get_num_threads();
		int num_items_per_thread = data.size() / num_threads;
		int start_index = thread_id * num_items_per_thread;
		int end_index = start_index + num_items_per_thread;
		// the last element possibly doesn't have a full num_items_per_thread elements
		if (thread_id == num_threads - 1)
			end_index = data.size();

#pragma omp critical
		{
			std::cout << "Thread #" << thread_id << ": processing " << (end_index - start_index) << " items from " << start_index << " to " << end_index << ".\n";
		}

		int id_sum = 0;
		double age_sum = 0;
		for (int i = start_index; i < end_index; i++)
		{
			PersonWithChangedData p_changed = modify_person_data(data[i]);
			if (p_changed.id < 0)
			{
				id_sum += p_changed.id;
				age_sum += p_changed.age;
				sorted_monitor.addItemSorted(p_changed);
			}
		}

#pragma omp atomic
		full_id_sum += id_sum;
#pragma omp atomic
		full_age_sum += age_sum;
	}

	save_persons_table(data, results_file_name, "Original people's data", false);
	save_modified_persons_table(sorted_monitor.getItems(), results_file_name, "Modified people's data, filtered by ID, sorted by age", true, full_id_sum, full_age_sum);
	return 0;
}