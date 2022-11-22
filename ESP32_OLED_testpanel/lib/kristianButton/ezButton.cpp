#include <kristianButton.h>

// husk å legg til "pinMode(BUTTON_PIN, INPUT_PULLUP);"
kristianButton::kristianButton(int pin)
{
	int btnPin = pin;
	int debounceTime = 0;
	int lastSteadyState = LOW;		// the previous steady state from the input pin
	int lastFlickerableState = LOW; // the previous flickerable state from the input pin
	int currentState;				// the current reading from the input pin
	// denne endres med setLoop funksjon
	bool theButtonState = false;

	// the following variables are unsigned longs because the time, measured in
	// milliseconds, will quickly become a bigger number than can be stored in an int.
	unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
}

bool kristianButton::buttonState(void)
{
	if (theButtonState == true)
	{
		return true;
	}
	else if (theButtonState == false)
	{
		return false;
	}
}

void kristianButton::debounce(int time)
{
	debounceTime = time;
}

void kristianButton::setLoop(void)
{
	// leser av status på knapp (denne vil også avlese støy)
	currentState = digitalRead(btnPin);

	// denne sjekker hele tiden om det er støy
	// altså om current state har endret seg raskere enn debounce
	if (currentState != lastFlickerableState)
	{
		// hvis "støy" -> reset debounce
		lastDebounceTime = millis();
		// lagrer dette resultatet for å sjekke neste gang
		lastFlickerableState = currentState;
	}

	if ((millis() - lastDebounceTime) > debounceTime)
	{
		// hvis du kommer inn i denne funksjonen, så har avlesningen vært på
		// lenger enn debounce tid, ergo:

		// hvis currentstate har endret seg:
		if (lastSteadyState == HIGH && currentState == LOW)
		{
			theButtonState = true;
		}
		else if (lastSteadyState == LOW && currentState == HIGH)
		{
			theButtonState = false;
		}

		// lagrer lastSteadyState
		lastSteadyState = currentState;
	}
	Serial.println(lastSteadyState);
}