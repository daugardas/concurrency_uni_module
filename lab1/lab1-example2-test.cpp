/*
  Monitor with integers c=10, d=100
  first and second processes change data (c++, d--)

  third, forth and fifth processes read data in monitor and if c an d weren't yet printed, prints it.

  first and second process can change c and d if they there read at least two times

 */

#include <iostream>;
#include <vector>;
#include <thread>;
#include <mutex>;
#include <condition_variable>;

class CDMonitor
{
public:
  CDMonitor()
  {
  }
  void addC()
  {
    std::unique_lock<std::mutex> lock(this->mtx);
    cv.wait(lock, [&]
            { return this->c_read >= 2; });
    this->c++;
    this->c_printed = 0;
    this->c_read = 0;
  }
  void subtractD()
  {
    std::unique_lock<std::mutex> lock(this->mtx);
    cv.wait(lock, [&]
            { return this->d_read >= 2; });
    this->d--;
    this->c_printed = 0;
    this->c_read = 0;
  }

  void read()
  {
    std::unique_lock<std::mutex> lock(this->mtx);
    if (this->c_printed == 0 && this->d_printed)
    {
      std::cout << this->c << " " << this->d << std::endl;
      this->c_printed++;
      
    }
    this->c_read++;
    this->cv.notify_all();
  }

  bool is_done()
  {
    std::unique_lock<std::mutex> lock(this->mtx);
    if (this->c >= 100 && this->d >= 100)
      return true;
    return false;
  }

private:
  int c = 10;
  int d = 100;
  int c_printed = 0;
  int c_read = 0;
  int d_printed = 0;
  int d_read = 0;
  int couples_printed = 0;

  std::mutex mtx;
  std::condition_variable cv;
};

void add_worker(CDMonitor &monitor)
{
  while (true)
  {
    monitor.addC();
  }
}

void subtract_worker(CDMonitor &monitor)
{
  while (true)
  {
    monitor.subtractD();
  }
}

void read_print_worker(CDMonitor &monitor)
{
  while (true)
  {
    monitor.readC();
    monitor.readD();
  }
}

int main()
{
  CDMonitor monitor;

  std::vector<std::thread> threads;
  threads.emplace_back()
}