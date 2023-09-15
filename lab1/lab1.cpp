#include <iostream>
#include <vector>
#include <fstream>
#include <thread>
#include <random>
#include <condition_variable>

#include "json.hpp"
using json = nlohmann::json;

using namespace std;

bool data_exists = false; // worker thread will check this variable to determine if it should finish its job
mutex mtx;
condition_variable cv;

struct Person
{
	int id;
	double age;
	string name;
};

struct PersonWithChangedData
{
	Person originalData;
	int id;
	double age;
	string name;
};

class DataMonitor
{
public:
	DataMonitor(int size)
	{
		if (size < 1)
		{
			throw runtime_error("Incorrect initial given size to a DataMonitor. Initial size has to be at least 1.");
		}

		persons = new Person[size];
		this->size = size;
		this->size_used = 0;
	}
	~DataMonitor()
	{
		delete[] persons;
	}
	int addItem(Person item)
	{
		unique_lock<mutex> lock(monitor_mtx);
		if (size_used == size)
		{
			return -1; // DataMonitor is full
		}

		persons[size_used] = item;
		size_used++;
		return 0;
	}
	Person removeItem()
	{
		unique_lock<mutex> lock(monitor_mtx);
		if (size_used > 0)
		{
			size_used--;
			return persons[size_used];
		}
		throw runtime_error("DataMonitor is empty. Before using removeItem() please check if it is not empty.");
	}
	bool is_full()
	{
		unique_lock<mutex> lock(monitor_mtx);
		return size == size_used;
	}
	bool is_empty()
	{
		unique_lock<mutex> lock(monitor_mtx);
		return size_used == 0;
	}

private:
	Person *persons;
	int size;
	int size_used;
	mutex monitor_mtx;
};

class SortedResultMonitor
{
public:
	SortedResultMonitor(int size)
	{
		if (size < 1)
		{
			throw runtime_error("Incorrect initial given size to a SortedResultMonitor. Initial size has to be at least 1.");
		}

		persons = new PersonWithChangedData[size];
		this->size = size;
		this->size_used = 0;
	}
	~SortedResultMonitor()
	{
		delete[] persons;
	}
	int addItemSorted(PersonWithChangedData item)
	{
		unique_lock<mutex> lock(monitor_mtx);
		if (size_used == size)
		{
			return -1; // SortedResultMonitor is full
		}

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
		return 0;
	}
	vector<PersonWithChangedData> getItems()
	{
		vector<PersonWithChangedData> items;
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
	mutex monitor_mtx;
};

void save_modified_persons(const std::vector<PersonWithChangedData> &data, const std::string &file_name)
{
	cout << "saving " << data.size() << " modified persons.\n";

	ofstream o(file_name);
	if (data.size() > 0)
	{
		o << "_________________________________________________________" << endl;
		o << "| Modified people's data, filtered by ID, sorted by age |" << endl;
		o << "|-------------------------------------------------------|" << endl;
		o << "| ID          | Name                           | Age    |" << endl;
		o << "|-------------------------------------------------------|" << endl;
		for (auto &p : data)
		{
			o << "| " << setw(11) << p.id << " | " << setw(30) << p.name << " | " << setw(6) << p.age << " |" << endl;
		}
		o << "|-------------------------------------------------------|" << endl;
		o << endl;
		o << "_________________________________________________________" << endl;
		o << "| Original data of the modified people's data           |" << endl;
		o << "|-------------------------------------------------------|" << endl;
		o << "| ID          | Name                           | Age    |" << endl;
		o << "|-------------------------------------------------------|" << endl;
		for (auto &p : data)
		{
			o << "| " << setw(11) << p.originalData.id << " | " << setw(30) << p.originalData.name << " | " << setw(6) << p.originalData.age << " |" << endl;
		}
		o << "|-------------------------------------------------------|" << endl;
	}
	else
	{
		o << "No modified people's data. Either there was no data to begin with, or all of it was filtered." << endl;
	}
	o.close();
}

vector<Person> load_json_file(const std::string &file_name)
{
	std::ifstream f(file_name);
	if (!f.is_open())
	{
		cerr << "Failed to open given '" << file_name << "' file." << endl;
		return vector<Person>();
	}
	json data = json::parse(f);
	vector<Person> data_vector;
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
	// p.age *= 0.000000000001;

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

void worker_thread(DataMonitor &data_monitor, SortedResultMonitor &sorted_result_monitor)
{
	while (true)
	{
		Person p;
		// lock this thread until data_monitor has data or if the data_exists variable becomes false
		{
			unique_lock<mutex> lock(mtx);
			cv.wait(lock, [&data_monitor]
					{ return !data_monitor.is_empty() || !data_exists; });

			if (!data_exists && data_monitor.is_empty())
			{
				cout << "Thread #" << this_thread::get_id() << ": there will not be data added anymore. Stopping work." << endl;
				break;
			}

			p = data_monitor.removeItem();
		}
		cv.notify_all(); // notify that there is an item removed from the data_monitor

		PersonWithChangedData p_changed = modify_person_data(p);
		if (p_changed.id < 0)
		{
			cout << endl
				 << "Thread #" << this_thread::get_id() << ": adding modified item to sorted results monitor." << endl;
			sorted_result_monitor.addItemSorted(p_changed);
		}
	}
}

int main()
{
	string file_name = "filters_some.json";
	string results_file_name = "results.txt";
	vector<Person> data = load_json_file(file_name);
	cout << "Loaded " << data.size() << " persons from '" << file_name << "'." << endl;
	data_exists = data.size() > 0;
	if (!data_exists)
	{
		cerr << "There is no data in '" + file_name + "'. Closing the program." << endl;
		return 1;
	}

	DataMonitor data_monitor(data.size() / 2 - 1);
	SortedResultMonitor sorted_monitor(data.size());

	// Create worker threads that will check if data exists then
	// it will wait until that data gets added to the DataMonitor.
	// For this task there has to be 2 <= x <= n/4 threads,
	// with n being the number of Person objects in the data vector.
	// Here the x will be a random number between 2 and either
	// n/4 or max number of threads the machine can handle.
	const int max_threads = thread::hardware_concurrency() - 1 > data.size() / 4 ? thread::hardware_concurrency() - 1 : data.size();
	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<int> dist(2, max_threads);
	const int num_threads = dist(gen);
	// const int num_threads = 2; // for testing purposes

	vector<thread> threads;
	for (int i = 0; i < num_threads; i++)
	{
		threads.emplace_back(worker_thread, ref(data_monitor), ref(sorted_monitor));
	}

	cout << endl
		 << "Main thread: created threads." << endl;

	for (auto &p : data)
	{
		// lock the main thread until there is space in the data_monitor
		{
			unique_lock<mutex> lock(mtx);
			cv.wait(lock, [&data_monitor]
					{ return !data_monitor.is_full(); });
			cout << endl
				 << "Main thread: adding a person to data monitor." << endl;
			data_monitor.addItem(p);
		}
		cv.notify_all(); // notify that there is an item added to the data_monitor
	}

	cout << "Main thread: no more data to work with, notifying worker threads." << endl;

	// notify worker threads that there is no more data to work with
	{
		lock_guard<mutex> lock(mtx);
		data_exists = false;
	}
	cv.notify_all();

	cout << "Main thread: waiting for threads to join." << endl;

	for (auto &thread : threads)
	{
		thread.join();
	}

	cout << "Main thread: threads joined, printing out the results to " << results_file_name << "." << endl;

	save_modified_persons(sorted_monitor.getItems(), results_file_name);
	return 0;
}