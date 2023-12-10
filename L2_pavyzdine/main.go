package main

import "fmt"

/*
	- Du procesai-siuntejai siunčia vienam procesui-gavejui skaičius, pirmasis procesas iš eilės nuo 0, antrasis
	nuo 11.

	- Procesas-gavėjas priima skaičius, ir siunčia juos vienam iš dviejų procesų-spausdintojų: pirma-
	jam lyginius, antrajam nelyginius.

	- Procesai-spausdintojai kaupia gautus skaičius savo masyvuose ir, abiem
	procesams-siuntejams baigus darbą, išspausdina savo masyvu turinius į ekraną.

	Darbas baigiamas, kai procesas-gavėjas priima ir persiunčia 20 skaičiu
*/

// Siuntėjas
func sender(number int, mainFinishedStatusCh <-chan bool, senderToMainCh chan<- int, senderFinishedStatusCh chan<- bool) {
	fmt.Println("Sender worker started.")
	defer close(senderFinishedStatusCh)

	for {
		select {
		case _, ok := <-mainFinishedStatusCh:
			fmt.Printf("mainFinishedStatusCh ok = %t\n", ok)
			if !ok {
				return
			}
			break

		default:
			fmt.Printf("Sending number to main.\n")
			senderToMainCh <- number
			number++
			break
		}
	}
}

// Spausdintojas
func printer(id int, printerFinished chan<- bool, sender1FinishedStatusCh <-chan bool, sender2FinishedStatusCh <-chan bool, mainToPrintCh <-chan int) {
	fmt.Println("Printer worker started.")
	sender1Finished := false
	sender2Finished := false

	receivedNumbers := make([]int, 0, 1)

	for {
		if !(sender1Finished && sender2Finished) {
			select {
			case _, ok := <-sender1FinishedStatusCh:
				fmt.Println("Sender 1 finished.")
				if !ok {
					sender1Finished = true
				}
				break
			case _, ok := <-sender2FinishedStatusCh:
				fmt.Println("Sender 2 finished.")
				if !ok {
					sender2Finished = true
				}
				break
			case number, ok := <-mainToPrintCh:
				fmt.Printf("received '%d' from main.\n", number)
				if ok {
					receivedNumbers = append(receivedNumbers, number)
				}
				break
			}
		} else {
			break
		}
	}

	// both senders are finished, so print array to terminal
	for i, number := range receivedNumbers {
		fmt.Printf("Printer %d: number %d = %d\n", id, i, number)
	}
	close(printerFinished)
}

// Gavėjas
func main() {
	fmt.Println("Creating sender channels.")

	mainFinishedStatusCh := make(chan bool)
	senderToMainCh := make(chan int)
	mainToEvenPrintCh := make(chan int)
	mainToUnevenPrintCh := make(chan int)
	sender1FinishedStatusCh := make(chan bool)
	sender2FinishedStatusCh := make(chan bool)
	printer1FinishedCh := make(chan bool)
	printer2FinishedCh := make(chan bool)

	fmt.Println("Creating sender Goroutines.")

	go sender(0, mainFinishedStatusCh, senderToMainCh, sender1FinishedStatusCh)
	go sender(11, mainFinishedStatusCh, senderToMainCh, sender2FinishedStatusCh)

	fmt.Println("Creating printer Goroutines.")

	go printer(0, printer1FinishedCh, sender1FinishedStatusCh, sender2FinishedStatusCh, mainToEvenPrintCh)
	go printer(1, printer2FinishedCh, sender1FinishedStatusCh, sender2FinishedStatusCh, mainToUnevenPrintCh)

	fmt.Println("Started listening for numbers from senders.")
	sentToPrinters := 0
	for {
		number, ok := <-senderToMainCh
		if ok {
			if number%2 == 0 {
				mainToEvenPrintCh <- number
			} else {
				mainToUnevenPrintCh <- number
			}
			sentToPrinters++
		}

		if sentToPrinters >= 20 {
			fmt.Println("Closing mainFinishedStatusCh channel")
			close(mainFinishedStatusCh)
			break
		}
	}

	fmt.Println("Waiting for printers to finish.")
	printer1Finished := false
	printer2Finished := false
	for {
		select {
		case _, ok := <-printer1FinishedCh:
			if !ok {
				printer1Finished = true
			}
			break
		case _, ok := <-printer2FinishedCh:
			if !ok {
				printer2Finished = true
			}
			break
		}

		if printer1Finished && printer2Finished {
			break
		}
	}
	fmt.Println("Main finished.")
}
