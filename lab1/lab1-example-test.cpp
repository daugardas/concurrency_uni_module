/*
  Monitor has a string type variable (initial value is "*").
  Three processes constantly are writing to that variable.
  First process writes "A", second process writes "B", third process writes "C".

  A consonant ("B" and "C") can only be added when at least the last letters are vowels ("A").
  Main process prints the string when it is changed and at least once other threads finish their work.

  All threads finish their work when one of them first reaches 15 characters.

 */

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>
#include <cctype>
#include <chrono>

class StringMonitor
{
public:
  StringMonitor()
  {
    this->text = "*";
    this->vowelCountSinceLastConsonant = 0;
    this->finishedWork = false;
  }

  void addCharacter(const char &ch)
  {
    // char upperCh = toupper(ch);
    std::unique_lock<std::mutex> lock(this->mtx);
    if (ch == 'A' || ch == 'E' || ch == 'U' || ch == 'I' || ch == 'Y' || ch == 'O')
    {
      this->vowelCountSinceLastConsonant++;
      this->text += ch;
    }
    else
    {
      // check if there are at least 3 vowels
      // if not, wait till there are.
      cv.wait(lock, [&]
              { return vowelCountSinceLastConsonant >= 3 || this->finishedWork; });

      if (this->finishedWork)
        return;

      this->text += ch;
      this->vowelCountSinceLastConsonant = 0;
    }
    // std::cout << this->text << std::endl;
    // std::cout << this->vowelCountSinceLastConsonant << std::endl;
    lock.unlock();
    cv.notify_all();
  }

  void finishWork()
  {
    std::unique_lock<std::mutex> lock(this->mtx);
    this->finishedWork = true;
  }

  bool isFinished()
  {
    std::unique_lock<std::mutex> lock(this->mtx);
    return this->finishedWork;
  }

  std::string getCurrentText()
  {
    std::unique_lock<std::mutex> lock(this->mtx);
    return this->text;
  }

private:
  std::string text;
  std::mutex mtx;
  std::condition_variable cv;
  int vowelCountSinceLastConsonant;
  bool finishedWork;
};

void worker_thread(StringMonitor &monitor, const char &ch)
{
  int addedCharacters = 0;
  while (true)
  {

    if (monitor.isFinished())
      return;

    monitor.addCharacter(ch);
    addedCharacters++;

    if (addedCharacters == 15)
    {
      monitor.finishWork();
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

int main()
{
  StringMonitor monitor;

  std::vector<std::thread> threads;
  threads.emplace_back(worker_thread, std::ref(monitor), (const char &)'B'); //, std::ref(monitor), std::ref('A'));
  threads.emplace_back(worker_thread, std::ref(monitor), (const char &)'A');
  threads.emplace_back(worker_thread, std::ref(monitor), (const char &)'C');

  // every second print out the current text in the monitor
  while (!monitor.isFinished())
    std::cout << "Current StringMonitor text: " << monitor.getCurrentText() << std::endl;

  for (auto &thread : threads)
    thread.join();

  std::cout << "Finished StringMonitor text: " << monitor.getCurrentText() << std::endl;
}