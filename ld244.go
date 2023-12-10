package aaa

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

func resultThread(sendComputedResultsChannel <-chan PersonWithChangedData, sendResultsChannel chan<- PersonWithChangedData) {
	defer close(sendResultsChannel)
	fmt.Println("result thread started")
	for {
		fmt.Println("Result thread is expecting a modified person data.")
		modifiedPerson, ok := <-sendComputedResultsChannel
		fmt.Printf("sendComputedResultsChannel is closed: %t\n", !ok)
		if !ok {
			fmt.Println("There will be no more computed results. Results thread is quiting.")
			break
		}
		fmt.Printf("Got this modified person from worker thread: \n")
		fmt.Printf("ID: %d, Name: %s, Age: %f\n\n", modifiedPerson.ID, modifiedPerson.Name, modifiedPerson.Age)
		fmt.Printf("Sending modified person to main thread.\n")
		sendResultsChannel <- modifiedPerson
	}
}

func workerThread(getDataChannel <-chan Person, sendComputedResultsChannel chan<- PersonWithChangedData, group *sync.WaitGroup) {
	defer group.Done()

	fmt.Println("worker thread started")
	for {
		fmt.Println("Waiting for a person from data thread.")
		person, ok := <-getDataChannel
		fmt.Printf("Got this person from data thread: \n")
		fmt.Printf("ID: %d, Name: %s, Age: %f\n\n", person.ID, person.Name, person.Age)
		if !ok {
			break
		}
		fmt.Printf("Modifying person data.\n")
		modifiedPerson := ModifyPersonData(person)
		fmt.Printf("Sending modified person to results thread\n")
		sendComputedResultsChannel <- modifiedPerson
	}
}

func dataThread(sendDataChannel <-chan Person, getDataChannel chan<- Person) {
	defer close(getDataChannel)
	fmt.Println("data thread started")
	for {
		person, ok := <-sendDataChannel // sends items
		fmt.Printf("Got this person from main thread: \n")
		fmt.Printf("ID: %d, Name: %s, Age: %f\n\n", person.ID, person.Name, person.Age)
		if !ok {
			break
		}
		fmt.Printf("Sending person to worker threads")
		getDataChannel <- person // worker thread asks for items
	}
}

func main() {
	selectedDataFile := "all"
	fileName := "filters_" + selectedDataFile + ".json"
	resultsFileName := "results_" + selectedDataFile + ".txt"
	data := LoadJSONFile(fileName)
	fmt.Printf("Loaded %d persons from '%s'.\n", len(data), fileName)
	dataExists := len(data) > 0
	if !dataExists {
		panic(fmt.Sprintf("There is no data in '%s'. Closing the program.", fileName))
	}

	sendDataChannel := make(chan Person, len(data)/2)
	getDataChannel := make(chan Person, len(data))
	sendComputedResultsChannel := make(chan PersonWithChangedData)
	sendResultsChannel := make(chan PersonWithChangedData)

	workersGroup := sync.WaitGroup{}

	// choose a random amount of worker threads,
	// that will ask for persons, do stuff with them,
	// and if the result passes the criteria,
	// sends it to the result thread
	workerCount := rand.Intn(len(data)/4-2) + 2
	workersGroup.Add(workerCount)
	for i := 0; i < workerCount; i++ {
		go workerThread(getDataChannel, sendComputedResultsChannel, &workersGroup)
	}

	go dataThread(sendDataChannel, getDataChannel)

	// wait for worker threads to finish their jobs and close the sendComputedResultsChannel asynchroniously
	go func() {
		workersGroup.Wait()
		close(sendComputedResultsChannel)
	}()

	go resultThread(sendComputedResultsChannel, sendResultsChannel)

	// send data to data thread
	for _, person := range data {
		fmt.Printf("Sending the following person to data thread:\nID: %d, Name: %s, Age: %f\n\n", person.ID, person.Name, person.Age)
		sendDataChannel <- person
	}
	close(sendDataChannel) // no more data to send
	fmt.Println("closed data channel. Waiting for results.")

	// wait for all results to be computed
	results := make([]PersonWithChangedData, 0)
	for {
		fmt.Println("Waiting for a modified person from results channel.")
		person, ok := <-sendResultsChannel
		fmt.Printf("Got a person from results thread. Status: %t\n", !ok)
		if !ok {
			break
		}
		fmt.Printf("Got the following modified person from results thread person: \n")
		fmt.Printf("ID: %d, Name: %s, Age: %f\n\n", person.ID, person.Name, person.Age)
		results = append(results, person)
	}

	SavePersonsTable(data, resultsFileName, "Original people's data", false)
	SaveModifiedPersonsTable(results, resultsFileName, "Modified people's data, filtered by ID, sorted by age", true)
}
