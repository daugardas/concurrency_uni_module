#include <iostream>
#include <vector>
#include <fstream>
#include <thread>
#include <random>
#include <condition_variable>

#include "json.hpp"
using json = nlohmann::json;

std::condition_variable cv;

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

class DataMonitor
{
public:
	DataMonitor(int size, bool &data_exists)
	{
		if (size < 1)
		{
			throw std::runtime_error("Incorrect initial given size to a DataMonitor. Initial size has to be at least 1.");
		}

		if (!data_exists)
			throw std::runtime_error("DataMonitor cannot be created if there is no data to begin with.");

		persons = new Person[size];
		this->size = size;
		this->size_used = 0;
		this->data_exists = data_exists;
	}
	~DataMonitor()
	{
		delete[] persons;
	}
	void notify_workers_no_data()
	{
		{
			std::lock_guard<std::mutex> lock(monitor_mtx);
			data_exists = false;
		}
		cv.notify_all();
	}

	void addItem(Person item)
	{
		{
			std::unique_lock<std::mutex> lock(monitor_mtx);
			cv.wait(lock, [this]
					{ return size_used < size; }); // wait until there is space in the data_monitor
			persons[size_used] = item;
			size_used++;
		}
		cv.notify_all(); // notify that there is an item added to the data_monitor
	}
	Person removeItem()
	{
		{
			std::unique_lock<std::mutex> lock(monitor_mtx);
			cv.wait(lock, [this]
					{ return size_used > 0 || !data_exists; });
			if (!data_exists && size_used == 0)
				return Person();

			if (size_used > 0)
				size_used--;
		}
		cv.notify_all(); // notify that there is an item removed from the data_monitor
		return persons[size_used];
	}
	bool is_full()
	{
		std::unique_lock<std::mutex> lock(monitor_mtx);
		return size == size_used;
	}
	bool is_empty()
	{
		std::unique_lock<std::mutex> lock(monitor_mtx);
		return size_used == 0;
	}

	int get_size()
	{
		std::unique_lock<std::mutex> lock(monitor_mtx);
		return size;
	}

private:
	Person *persons;
	int size;
	int size_used;
	std::mutex monitor_mtx;
	bool data_exists;
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
	int addItemSorted(PersonWithChangedData item)
	{
		{
			std::unique_lock<std::mutex> lock(monitor_mtx);
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
		}
		cv.notify_all();
		return 0;
	}
	std::vector<PersonWithChangedData> getItems()
	{
		std::unique_lock<std::mutex> lock(monitor_mtx);
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
	std::mutex monitor_mtx;
	bool data_exists;
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

void save_modified_persons_table(const std::vector<PersonWithChangedData> &data, const std::string &file_name, const std::string &title, const bool &append)
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
		o << std::endl;
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
		Person p = data_monitor.removeItem();
		if (p.name.length() == 0)
		{
			std::cout << "Thread #" << std::this_thread::get_id() << ": there will not be data added anymore. Stopping work." << std::endl;
			break;
		}

		PersonWithChangedData p_changed = modify_person_data(p);
		if (p_changed.id < 0)
		{
			std::cout << std::endl
					  << "Thread #" << std::this_thread::get_id() << ": adding modified item to sorted results monitor." << std::endl;
			sorted_result_monitor.addItemSorted(p_changed);
		}
	}
}

int main()
{
	std::string file_name = "filters_some.json";
	std::string results_file_name = "results.txt";
	std::vector<Person> data = load_json_file(file_name);
	std::cout << "Loaded " << data.size() << " persons from '" << file_name << "'." << std::endl;
	bool data_exists = data.size() > 0;
	if (!data_exists)
	{
		std::cerr << "There is no data in '" + file_name + "'. Closing the program." << std::endl;
		return 1;
	}

	DataMonitor data_monitor(data.size() / 2 - 1, std::ref(data_exists));
	SortedResultMonitor sorted_monitor(data.size());

	// Create worker threads that will check if data exists then
	// it will wait until that data gets added to the DataMonitor.
	// For this task there has to be 2 <= x <= n/4 threads,
	// with n being the number of Person objects in the data vector.
	// Here the x will be a random number between 2 and either
	// n/4 or max number of threads the machine can handle.
	const int max_threads = std::thread::hardware_concurrency() - 1 > data.size() / 4 ? std::thread::hardware_concurrency() - 1 : data.size();
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(2, max_threads);
	const int num_threads = dist(gen);
	// const int num_threads = 2; // for testing purposes

	std::vector<std::thread> threads;
	for (int i = 0; i < num_threads; i++)
	{
		threads.emplace_back(worker_thread, std::ref(data_monitor), std::ref(sorted_monitor));
	}

	std::cout << std::endl
			  << "Main thread: created threads." << std::endl;

	for (auto &p : data)
	{
		std::cout << std::endl
				  << "Main thread: adding a person to data monitor." << std::endl;
		data_monitor.addItem(p);
	}

	std::cout << "Main thread: there are " << data_monitor.get_size() << " items left in the data monitor." << std::endl;

	data_monitor.notify_workers_no_data();

	std::cout << "Main thread: waiting for threads to join." << std::endl;

	for (auto &thread : threads)
	{
		thread.join();
	}

	std::cout << "Main thread: threads joined, printing out the results to " << results_file_name << "." << std::endl;

	save_persons_table(data, results_file_name, "Original people's data", false);
	save_modified_persons_table(sorted_monitor.getItems(), results_file_name, "Modified people's data, filtered by ID, sorted by age", true);
	return 0;
}