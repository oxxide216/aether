const chat = document.querySelector('.chat');
const result = document.querySelector('.result');
const warn = document.querySelector('.warn');
const error = document.querySelector('.error');

let prompt = document.querySelector('.prompt');
let input = prompt.querySelector('.prompt-input');

function setTextAnimated(element, text) {
  let i = 0;
  const typeChar = () => {
    if (i < text.length) {
      let char = text.charAt(i);
      if (char == '\n')
        char = '<br/>'
      element.innerHTML += char;
      ++i;
      element.scrollIntoView({ block: 'nearest' });
      setTimeout(typeChar, 10);
    }
  };

  typeChar();
}

function handler(event) {
  if (event.key == 'Enter' && this.value.trim() != '') {
    event.preventDefault();

    const resultText = aetherEval(input.value);
    if (resultText.length > 0 && resultText != 'unit') {
      const newResult = result.cloneNode(true);
      newResult.setAttribute("style", "");
      chat.appendChild(newResult);

      setTextAnimated(newResult, resultText);
    }

    prompt = prompt.cloneNode(true);
    chat.appendChild(prompt);

    input.setAttribute("disabled", "true");
    input.onkeypress = null;

    input = prompt.querySelector('.prompt-input');
    input.onkeypress = handler;
    input.value = '';
    input.focus();
    input.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }
};

const originalLog = console.log;
console.log = (...args) => {
  originalLog.apply(console, args);
  const text = args.join(' ');

  if (text.trim().length == 0)
    return;

  const newLog = result.cloneNode(true);
  newLog.setAttribute("style", "");
  chat.appendChild(newLog);

  setTextAnimated(newLog, text);
};

const originalWarn = console.warn;
console.warn = (...args) => {
  originalWarn.apply(console, args);
  const text = args.join(' ');

  if (text.trim().length == 0)
    return;

  const newWarn = warn.cloneNode(true);
  newWarn.setAttribute("style", "");
  chat.appendChild(newWarn);

  setTextAnimated(newWarn, text);
};

const originalError = console.error;
console.error = (...args) => {
  originalError.apply(console, args);
  const text = args.join(' ');

  if (text.trim().length == 0)
    return;

  if (!text.startsWith('program exited')) {
    const newError = error.cloneNode(true);
    newError.setAttribute("style", "");
    chat.appendChild(newError);

    setTextAnimated(newError, text);
  }
};

document.addEventListener('click', () => input.focus());
input.focus();

aetherInit('.', () => {
  input.onkeypress = handler;
});
