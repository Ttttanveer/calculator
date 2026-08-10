let expression = "";

const expressionDisplay =
    document.getElementById("expression");

const resultDisplay =
    document.getElementById("result");


function updateDisplay()
{
    expressionDisplay.textContent =
        expression;

    if (expression === "")
    {
        resultDisplay.textContent = "0";
    }
}


/* ============================
   ADD NUMBER
   ============================ */

function addNumber(number)
{
    expression += number;

    updateDisplay();
}


/* ============================
   ADD OPERATOR
   ============================ */

function addOperator(operator)
{
    if (expression === "")
        return;

    expression += operator;

    updateDisplay();
}


/* ============================
   CLEAR
   ============================ */

function clearCalculator()
{
    expression = "";

    expressionDisplay.textContent = "";

    resultDisplay.textContent = "0";
}


/* ============================
   BACKSPACE
   ============================ */

function backspace()
{
    expression =
        expression.slice(0, -1);

    updateDisplay();
}


/* ============================
   CALCULATE
   ============================ */

async function calculate()
{
    if (expression === "")
        return;

    expressionDisplay.textContent =
        expression;

    try
    {
        const response =
            await fetch("/calculate",
            {
                method: "POST",

                headers:
                {
                    "Content-Type":
                        "application/x-www-form-urlencoded"
                },

                body:
                    "expression=" +
                    encodeURIComponent(expression)
            });


        const data =
            await response.json();


        if (data.success)
        {
            resultDisplay.textContent =
                data.result;

            /*
             * Keep the result as the
             * next expression.
             */

            expression =
                String(data.result);
        }
        else
        {
            resultDisplay.textContent =
                "Error";
        }

    }
    catch (error)
    {
        resultDisplay.textContent =
            "Server Error";
    }
}


/* ============================
   KEYBOARD SUPPORT
   ============================ */

document.addEventListener(
    "keydown",
    function(event)
    {
        const key = event.key;


        if (
            (key >= "0" && key <= "9") ||
            key === "."
        )
        {
            addNumber(key);
        }


        else if (
            key === "+" ||
            key === "-" ||
            key === "*" ||
            key === "/"
        )
        {
            addOperator(key);
        }


        else if (key === "Enter")
        {
            calculate();
        }


        else if (key === "Backspace")
        {
            backspace();
        }


        else if (key === "Escape")
        {
            clearCalculator();
        }
    }
);