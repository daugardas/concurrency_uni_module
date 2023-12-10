package maintest

import (
	"encoding/json"
	"fmt"
	"math"
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

func WorkerThread(requestRemovePersonFromDataCh chan<- bool, getDataCh <-chan Person, sendDataCh chan<- PersonWithChangedData, wg *sync.WaitGroup) {
	defer wg.Done()
	fmt.Println("Created Worker thread.")
	for {
		select {
		case person, ok := <-getDataCh:
			fmt.Println("Worker thread: received person.")
			if ok {
				modifiedPerson := ModifyPersonData(person)
				fmt.Println("Worker thread: modified person.")
				if modifiedPerson.ID < 0 {
					fmt.Println("Worker thread: sent.")
					sendDataCh <- modifiedPerson
				} else {
					fmt.Println("Worker thread: not sent.")
					//break
				}
			} else {
				fmt.Println("Worker thread: finishing work.")
				return
			}
		default:
			fmt.Println("Worker thread: requesting person from data thread.")
			requestRemovePersonFromDataCh <- true
			fmt.Println("Worker thread: requested.")
		}
	}

}

func ResultsThread(size int, receiveComputedPersonCh <-chan PersonWithChangedData, sendResultsCh chan<- PersonWithChangedData) {
	defer close(sendResultsCh)
	fmt.Println("Created Results thread.")
	data := make([]PersonWithChangedData, 0, size)
	wontReceiveMorePersons := false
	for {
		//fmt.Println("Results thread: waiting for a computed person to be sent over from a worker thread.")
		select {
		case person, ok := <-receiveComputedPersonCh:
			fmt.Printf("Results thread: a modified person was sent over. ID: %d, Name: %s, Age: %f\n", person.ID, person.Name, person.Age)
			if ok {
				data = append(data, person)
			} else {
				fmt.Println("Results thread: received a request to stop receiving persons.")
				wontReceiveMorePersons = true
			}
		default:
			if wontReceiveMorePersons && len(data) == 0 {
				fmt.Println("Results thread: received a request to stop receiving persons and there are no more persons to receive. Closing the thread.")
				return
			}
			//fmt.Printf("Results thread: data length = %d\n", len(data))
			if len(data) > 0 {
				person := data[0]
				data = data[1:]
				fmt.Printf("Results thread: sending results to main thread\n")
				sendResultsCh <- person
				fmt.Printf("Results thread: sent.")
			}
		}
	}

}

func DataThread(size int, dataThreadReadyForPersonCh chan<- bool, addPersonCh <-chan Person, requestRemovePersonFromDataCh <-chan bool, sendDataCh chan<- Person) {
	//defer close(sendDataCh)
	fmt.Println("Created Data thread.")
	data := make([]Person, 0, size)
	fmt.Printf("Data thread: initial data array size: %d\n", len(data))
	dataThreadReadyForPersonCh <- true
	wontReceiveMorePersons := false
	//workerThreadReadyForPerson := false
	for {
		//fmt.Println("Data Thread: waiting for a person or request to send person.")
		select {
		case person, ok := <-addPersonCh:
			if !ok {
				fmt.Println("Data Thread: received a request to stop receiving persons.")
				wontReceiveMorePersons = true
				break
			}
			fmt.Printf("Data Thread: received a person with status ok - %t\n", ok)
			fmt.Printf("Person:\nID: %d, Name: %s, Age: %f\n", person.ID, person.Name, person.Age)
			if len(data) < size {
				fmt.Println("Data thread: appending to array.")
				data = append(data, person)

				if len(data) < size {
					fmt.Println("Data thread: signaling main thread that it can send over a person")
					dataThreadReadyForPersonCh <- true
				} else {
					fmt.Println("Data thread: appended. No more space in data thread, main thread will have to wait.")
					dataThreadReadyForPersonCh <- false
				}
			} else {
				fmt.Println("Data Thread: data array is full.")
				dataThreadReadyForPersonCh <- false
			}
		case <-requestRemovePersonFromDataCh:
			fmt.Printf("Data Thread: received a request to send over a person to a worker thread. Current person count in thread: %d\n", len(data))
			if len(data) > 0 {
				person := data[0]
				data = data[1:]
				fmt.Printf("Data Thread: sending person to worker thread.\n")
				sendDataCh <- person
				fmt.Printf("Data Thread: sent.\n")
				dataThreadReadyForPersonCh <- true
			} else if wontReceiveMorePersons {
				fmt.Println("Data thread: wont receive any more persons and all persons have already been sent. Closing the thread and channel.")
				close(sendDataCh)
				return
			} else {
				fmt.Println("Data thread: how do I escape")
			}
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

	dataThreadReadyForPersonCh := make(chan bool)
	addPersonToDataCh := make(chan Person)
	requestRemovePersonFromDataCh := make(chan bool)
	dataCh := make(chan Person)
	sendComputedDataCh := make(chan PersonWithChangedData)
	sendResultsDataCh := make(chan PersonWithChangedData)

	workersGroup := sync.WaitGroup{}
	// choose a random amount of worker threads,
	// that will ask for persons, do stuff with them,
	// and if the result passes the criteria,
	// sends it to the result thread
	//workerCount := rand.Intn(len(data)/4-2) + 2
	workerCount := 11
	workersGroup.Add(workerCount)
	for i := 0; i < workerCount; i++ {
		go WorkerThread(requestRemovePersonFromDataCh, dataCh, sendComputedDataCh, &workersGroup)
	}

	// wait for all workers to be done with their work and close the sendComputedDataCh channel
	go func() {
		workersGroup.Wait()
		close(sendComputedDataCh)
	}()

	maxDataSizeForDataThread := len(data)/2 - 1
	if maxDataSizeForDataThread < 1 {
		maxDataSizeForDataThread = 1
	}
	go DataThread(maxDataSizeForDataThread, dataThreadReadyForPersonCh, addPersonToDataCh, requestRemovePersonFromDataCh, dataCh)
	go ResultsThread(len(data), sendComputedDataCh, sendResultsDataCh)

	fmt.Println("Main thread: Starting sending persons to data thread.")
	// send persons to data thread
	for _, person := range data {
		fmt.Println("Main thread: waiting for a signal from the data thread.")
		ready := <-dataThreadReadyForPersonCh
		fmt.Printf("Main thread: received a signal from the data thread. Ready = %t\n", ready)

		for !ready {
			fmt.Println("Main thread: waiting for a signal from the data thread.")
			ready = <-dataThreadReadyForPersonCh
			fmt.Printf("Main thread: received a signal from the data thread. Ready = %t\n", ready)
		}

		fmt.Printf("Main thread: sending a person with ID: %d, Name: %s, Age: %f\n", person.ID, person.Name, person.Age)
		addPersonToDataCh <- person
		fmt.Println("Main thread: sent.")
	}
	fmt.Println("Main thread: closing main-data-thread communication.")
	close(addPersonToDataCh)
	fmt.Println("Main thread: closed main-data threads communication")

	fmt.Println("Main thread: Sent all persons to data thread. Starting receiving modified persons.")

	// receive results from results thread
	results := make([]PersonWithChangedData, 0)
	for {
		person, ok := <-sendResultsDataCh
		fmt.Printf("Main thread: received a person from the results thread. Status = %t\n", ok)
		if ok {
			results = append(results, person)
		} else {
			break
		}
	}

	fmt.Println("Main thread: read all results. Printing out.")

	SavePersonsTable(data, resultsFileName, "Original people's data", false)
	SaveModifiedPersonsTable(results, resultsFileName, "Modified people's data, filtered by ID, sorted by age", true)
}
