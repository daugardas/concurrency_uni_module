package main

import (
	"encoding/json"
	"fmt"
	"math"
	"math/rand"
	"os"
	"sync"
)

type Person struct {
	ID   int     `json:"id"`
	Age  float64 `json:"age"`
	Name string  `json:"name"`
}

type PersonWithChangedData struct {
	OriginalData Person
	ID           int
	Age          float64
	Name         string
}

func SavePersonsTable(data []Person, fileName string, title string, append bool) {
	fmt.Printf("saving %d persons.\n", len(data))

	var o *os.File
	var err error
	if append {
		o, err = os.OpenFile(fileName, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	} else {
		o, err = os.Create(fileName)
	}
	if err != nil {
		panic(err)
	}
	defer func(o *os.File) {
		_ = o.Close()
	}(o)

	if len(data) > 0 {
		_, _ = o.WriteString("_________________________________________________________\n")
		_, _ = o.WriteString(fmt.Sprintf("| %-51s |\n", title))
		_, _ = o.WriteString("|-------------------------------------------------------|\n")
		_, _ = o.WriteString("| ID          | Name                           | Age    |\n")
		_, _ = o.WriteString("|-------------------------------------------------------|\n")
		for _, p := range data {
			_, _ = o.WriteString(fmt.Sprintf("| %-11d | %-30s | %-6.2f |\n", p.ID, p.Name, p.Age))
		}
		_, _ = o.WriteString("|-------------------------------------------------------|\n\n")
	} else {
		_, _ = o.WriteString("No people's data. Either there was no data to begin with, or all of it was filtered.\n")
	}
}

func SaveModifiedPersonsTable(data []PersonWithChangedData, fileName string, title string, append bool) {
	fmt.Printf("saving %d modified persons.\n", len(data))

	var o *os.File
	var err error
	if append {
		o, err = os.OpenFile(fileName, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	} else {
		o, err = os.Create(fileName)
	}
	if err != nil {
		panic(err)
	}
	defer func(o *os.File) {
		_ = o.Close()
	}(o)

	if len(data) > 0 {
		_, _ = o.WriteString("_________________________________________________________\n")
		_, _ = o.WriteString(fmt.Sprintf("| %-51s |\n", title))
		_, _ = o.WriteString("|-------------------------------------------------------|\n")
		_, _ = o.WriteString("| ID          | Name                           | Age    |\n")
		_, _ = o.WriteString("|-------------------------------------------------------|\n")
		for _, p := range data {
			_, _ = o.WriteString(fmt.Sprintf("| %-11d | %-30s | %-6.2f |\n", p.ID, p.Name, p.Age))
		}
		_, _ = o.WriteString("|-------------------------------------------------------|\n\n")
	} else {
		_, _ = o.WriteString("No modified people's data. Either there was no data to begin with, or all of it was filtered.\n")
	}
}

func LoadJSONFile(fileName string) []Person {
	f, err := os.Open(fileName)
	if err != nil {
		panic(fmt.Sprintf("Failed to open given '%s' file.", fileName))
	}
	defer func(f *os.File) {
		_ = f.Close()
	}(f)

	var data []Person
	err = json.NewDecoder(f).Decode(&data)
	if err != nil {
		panic(err)
	}

	return data
}

func ModifyPersonData(person Person) PersonWithChangedData {
	p := PersonWithChangedData{
		OriginalData: person,
	}

	// Generate a new id with very complex calculations
	p.ID = 0
	for i := 0; i < 1000000; i++ {
		p.ID += person.ID * i
		for j := 0; j < 100; j++ {
			p.ID += i * j
		}
	}
	p.ID /= 100000000

	// Generate a new age with very complex calculations
	p.Age = person.Age
	for i := 0; i < 1000000; i++ {
		if p.Age < 0 {
			p.Age += -p.Age * 3.1425
		} else {
			p.Age += p.Age * 3.1425
		}
		for j := 0; j < 100; j++ {
			p.Age += float64(i + j)
		}
		if p.Age < 0 || p.Age > 10000 {
			p.Age = person.Age
		}
	}

	// Generate a new name with very complex calculations
	p.Name = ""
	for i := 0; i < 10; i++ {
		randomChar := byte(math.Mod(float64(person.ID+i), 25) + 'A') // Generate 'A' to 'Z'
		p.Name += string(randomChar)
		for j := 0; j < 2; j++ {
			randomChar = byte(math.Mod(person.Age+float64(i*j), 25) + 'A') // Generate 'A' to 'Z'
			p.Name += string(randomChar)
		}
	}

	return p
}

func ResultThread(size int, sendModifiedPersonFromWorkerToResult <-chan PersonWithChangedData, sendModifiedPersonFromResultToMain chan<- PersonWithChangedData) {
	defer close(sendModifiedPersonFromResultToMain)
	data := make([]PersonWithChangedData, 0, size)
	noMoreWrites := false
	for {
		// if the data array is full or if there won't be anymore writes and there are still items in the data array, accept only reads
		if len(data) >= size || (noMoreWrites && len(data) > 0) {
			//fmt.Println("Result: sending modified person to Main.")
			person := data[0]
			data = data[1:]
			sendModifiedPersonFromResultToMain <- person
			//fmt.Println("Result: sent.")
			continue
		} else if noMoreWrites && len(data) == 0 {
			//fmt.Println("Result: no more writes and no more data. Exit result thread.")
			return
		}

		// if there is space in the data array accept both reads and writes
		//fmt.Println("Result: waiting for modified person from worker")
		person, ok := <-sendModifiedPersonFromWorkerToResult
		//fmt.Println("Result: response from worker")
		if !ok {
			noMoreWrites = true
			//fmt.Println("Result: no more writes.")
		} else {
			//fmt.Println("Result: append new person")
			data = append(data, person)
		}

	}
}

func WorkerThread(requestPersonFromDataToWorkerCh chan<- bool, sendPersonFromDataToWorkerCh <-chan Person, sendModifiedPersonFromWorkerToResult chan<- PersonWithChangedData, workersGroup *sync.WaitGroup) {
	defer workersGroup.Done()

	for {
		//fmt.Println("Worker: requesting or receiving person data")
		select {
		case requestPersonFromDataToWorkerCh <- true:
			//fmt.Println("Worker: request sent to data thread.")
			person, ok := <-sendPersonFromDataToWorkerCh
			//fmt.Println("Worker: response from data thread.")
			if !ok {
				//fmt.Println("Worker: no more data from data thread. Break out!")
				break
			}
			//fmt.Println("Worker: modifying person data.")
			modifiedPerson := ModifyPersonData(person)
			//fmt.Println("Worker: finished modifying.")
			if modifiedPerson.Age < 0 {
				//fmt.Println("Worker: sending modified person to result thread.")
				sendModifiedPersonFromWorkerToResult <- modifiedPerson
				//fmt.Println("Worker: sent.")
				continue
			}

			//fmt.Println("Worker: modification does not pass.")

		case person, ok := <-sendPersonFromDataToWorkerCh:
			//fmt.Println("Worker: response from result thread.")
			if !ok {
				//fmt.Println("Worker: no more data from result thread. Break out!")
				return
			}
			//fmt.Println("Worker: modifying person data.")
			modifiedPerson := ModifyPersonData(person)
			//fmt.Println("Worker: finished modifying.")
			if modifiedPerson.Age < 0 {
				//fmt.Println("Worker: sending modified person to result thread.")
				sendModifiedPersonFromWorkerToResult <- modifiedPerson
				//fmt.Println("Worker: sent.")
				continue
			}

			//fmt.Println("Worker: modification does not pass.")
		}
	}
}

func DataThread(maxDataSizeForDataThread int, sendPersonFromMainToDataCh <-chan Person, requestPersonFromDataToWorkerCh <-chan bool, sendPersonFromDataToWorkerCh chan<- Person) {
	defer close(sendPersonFromDataToWorkerCh)
	data := make([]Person, 0, maxDataSizeForDataThread)
	noMoreDataFromMain := false
	for {
		if len(data) < maxDataSizeForDataThread && len(data) > 0 && !noMoreDataFromMain {
			//fmt.Println("Data: waiting for a person from Main or a request from worker")
			select {
			case person, ok := <-sendPersonFromMainToDataCh:
				//fmt.Println("Data: main sent a person")
				if !ok {
					//fmt.Println("Data: no more data from main")
					noMoreDataFromMain = true
					continue
				}
				//fmt.Println("Data: append new person from main")
				data = append(data, person)
				break
			case <-requestPersonFromDataToWorkerCh:
				//fmt.Println("Data: request sent from worker")
				person := data[0]
				data = data[1:]
				//fmt.Println("Data: sending person to worker")
				sendPersonFromDataToWorkerCh <- person
				//fmt.Println("Data: sent.")
				break
			}
		} else if len(data) == maxDataSizeForDataThread || (len(data) > 0 && len(data) < maxDataSizeForDataThread && noMoreDataFromMain) {
			//fmt.Println("Data: waiting for request from a worker.")
			<-requestPersonFromDataToWorkerCh
			//fmt.Println("Data: request sent from a worker arrived.")
			person := data[0]
			data = data[1:]
			//fmt.Println("Data: sending person to worker.")
			sendPersonFromDataToWorkerCh <- person
			//fmt.Println("Data: sent.")
			continue
		} else if len(data) == 0 && !noMoreDataFromMain {
			//fmt.Println("Data: no data in thread, wait for data from main.")
			person, ok := <-sendPersonFromMainToDataCh
			//fmt.Println("Data: data arrived from main")
			if !ok {
				//fmt.Println("Data: no more data from main.")
				noMoreDataFromMain = true
				continue
			}
			//fmt.Println("Data: appending new data from main.")
			data = append(data, person)
		} else if len(data) == 0 && noMoreDataFromMain {
			//fmt.Println("Data: No more data from main and no more data to send to workers. Exiting Data thread.")
			return
		}
	}
}

func main() {
	selectedDataFile := "some"
	fileName := "filters_" + selectedDataFile + ".json"
	resultsFileName := "results_" + selectedDataFile + ".txt"
	data := LoadJSONFile(fileName)
	fmt.Printf("Loaded %d persons from '%s'.\n", len(data), fileName)
	dataExists := len(data) > 0
	if !dataExists {
		panic(fmt.Sprintf("There is no data in '%s'. Closing the program.", fileName))
	}

	fmt.Println("Creating channels.")

	sendPersonFromMainToDataCh := make(chan Person)
	requestPersonFromDataToWorkerCh := make(chan bool)
	sendPersonFromDataToWorkerCh := make(chan Person)
	sendModifiedPersonFromWorkerToResult := make(chan PersonWithChangedData)
	sendModifiedPersonFromResultToMain := make(chan PersonWithChangedData)

	workersGroup := sync.WaitGroup{}
	// choose a random amount of worker threads,
	// that will ask for persons, do stuff with them,
	// and if the result passes the criteria,
	// sends it to the result thread
	workerCount := rand.Intn(len(data)/4-2) + 2
	//workerCount := 5
	workersGroup.Add(workerCount)
	for i := 0; i < workerCount; i++ {
		go WorkerThread(requestPersonFromDataToWorkerCh, sendPersonFromDataToWorkerCh, sendModifiedPersonFromWorkerToResult, &workersGroup)
	}

	go func() {
		workersGroup.Wait() // wait until all workers finish their jobs and then close their channel
		close(sendModifiedPersonFromWorkerToResult)
	}()

	go ResultThread(len(data), sendModifiedPersonFromWorkerToResult, sendModifiedPersonFromResultToMain)
	maxDataSizeForDataThread := len(data)/2 - 1
	if maxDataSizeForDataThread < 1 {
		maxDataSizeForDataThread = 1
	}
	go DataThread(maxDataSizeForDataThread, sendPersonFromMainToDataCh, requestPersonFromDataToWorkerCh, sendPersonFromDataToWorkerCh)

	// send data to data thread
	for _, person := range data {
		//fmt.Println("Main: sending person to data thread.")
		sendPersonFromMainToDataCh <- person
		//fmt.Println("Main: sent.")
	}
	close(sendPersonFromMainToDataCh)

	fmt.Println("Main: Starting reading the results.")
	// read results from results thread
	results := make([]PersonWithChangedData, 0, len(data))
	for {
		//fmt.Println("Main: waiting for results thread to send something.")
		modifiedPerson, ok := <-sendModifiedPersonFromResultToMain
		//fmt.Println("Main: results thread sent sth.")
		if !ok {
			//fmt.Println("Main: results thread finished work.")
			break
		}
		//fmt.Println("Main: appending results.")
		results = append(results, modifiedPerson)
	}

	SavePersonsTable(data, resultsFileName, "Original people's data", false)
	SaveModifiedPersonsTable(results, resultsFileName, "Modified people's data, filtered by ID, sorted by age", true)
}
