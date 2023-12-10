#include <iostream>
#include <thread>
#include <vector>
#include <condition_variable>
#include <mutex>
/*
	Monitorius saugo sveikuju skaici

*/
using namespace std;
class IntMonitor {
public:
	IntMonitor() {
		this->initial_a = 10;
		this->a = this->initial_a;
		this->a_read_count = 0;
		this->total_read_count = 0;
		this->finished = false;
	}

	int readA(int prev_result, int thread_num) {
		unique_lock<mutex> lock(this->mtx);

		if (this->a == prev_result) {
			return this->a;
		}

		cout << this->a << ", thread_num: " << thread_num << endl;

		this->a_read_count++;
		this->total_read_count++;

		if (this->total_read_count >= 1000)
			this->finished = true;

		cv.notify_all();
		return this->a;
	}

	int readAVector(vector<int> prev_results, int thread_num) {
		unique_lock<mutex> lock(this->mtx);
		
		// check if prev_results has 'a' value
		bool already_read = false;
		for (auto& prev : prev_results) {
			if (prev == this->a)
			{
				already_read = true;
				break;
			}
		}

		if (already_read) {
			return -1;
		}

		cout << this->a << ", thread_num: " << thread_num << endl;

		this->a_read_count++;
		this->total_read_count++;

		if (this->total_read_count >= 20)
			this->finished = true;

		cv.notify_all();
		return this->a;
	}

	void writeA(int num) {
		unique_lock<mutex> lock(this->mtx);
		cv.wait(lock, [this] {
			return this->a_read_count >= 2 || finished;
			});

		if (finished)
			return;

		if (a_read_count == 3) {
			this->a = this->initial_a;
		}
		else {
			this->a = num;
		}

		this->a_read_count = 0;
		cv.notify_all();
	}

	bool isFinished() {
		unique_lock<mutex> lock(this->mtx);
		return finished;
	}
private:
	bool finished;
	int initial_a;
	int a;
	int a_read_count;
	int total_read_count;
	mutex mtx;
	condition_variable cv;
};

void write_thread(IntMonitor& monitor, int startFrom) {

	while (true) {
		if (monitor.isFinished())
			return;

		monitor.writeA(startFrom);

		startFrom++;
	}
}

void read_thread(IntMonitor& monitor, int num) {
	//vector<int> prev_results;
	int prev_result = -1;
	while (true) {
		if (monitor.isFinished())
			return;

		//int result = monitor.readAVector(prev_results, num);
		//prev_results.emplace_back(result);
		int result = monitor.readA(prev_result, num);
		prev_result = result;
	}
}

int main()
{
	IntMonitor monitor;
	vector<thread> threads;
	threads.emplace_back(write_thread, ref(monitor), 11);
	threads.emplace_back(write_thread, ref(monitor), 101);
	threads.emplace_back(read_thread, ref(monitor), 3);
	threads.emplace_back(read_thread, ref(monitor), 4);
	threads.emplace_back(read_thread, ref(monitor), 5);

	while (true) {
		if (monitor.isFinished())
			break;
	}

	for (auto& thread : threads)
		thread.join();

	cout << "Darbas baigtas" << endl;

	return 0;
}
