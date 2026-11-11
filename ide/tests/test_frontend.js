/*
 * Pruebas de integración del frontend con las correcciones LL(1).
 * Verifica LL1_INSERT, LL1_REPLACE, sugerencias obsoletas
 * y ausencia del mecanismo legado APPEND_BLOCK.
 */

const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");
const path = require("node:path");

const html = fs.readFileSync(
  path.join(__dirname, "..", "index.html"),
  "utf8"
);

const start = html.indexOf(
  "function applySuggestion(item) {"
);

const end = html.indexOf(
  "\nasync function checkSyntax(",
  start
);

assert.ok(
  start >= 0 && end > start,
  "No se encontró applySuggestion() en index.html"
);

const js = html.slice(start, end);

let checks = 0;

function simulate(
  input,
  issue,
  expected,
  expectedRuns = 1
) {
  const code = { value: input };
  let runs = 0;

  const ctx = {
    code,

    $: () => ({
      textContent: ""
    }),

    message: () => {},
    closeCompletions: () => {},
    renderIssues: () => {},
    redraw: () => {},

    checkSyntax: () => {
      runs++;
    },

    replaceEditorText: (start, end, text) => {
      code.value =
        code.value.slice(0, start) +
        text +
        code.value.slice(end);
    }
  };

  vm.runInNewContext(js, ctx);

  const applied = { ...issue };

  /*
   * Normalmente renderIssues() guarda el snapshot
   * original en _source. Lo simulamos aquí.
   */
  if (!Object.prototype.hasOwnProperty.call(applied, "_source")) {
    applied._source = input;
  }

  ctx.applySuggestion(applied);

  assert.equal(code.value, expected);
  assert.equal(runs, expectedRuns);

  checks++;
}

const base =
  "defina vacio principal() {\n" +
  " entero x = 1 :)\n" +
  "} :)\n";

const issue = (
  line,
  column,
  code,
  expected,
  replacement
) => ({
  line,
  column,
  code,
  expected,
  replacement
});


/* Falta :) */

simulate(
  base.replace("x = 1 :)", "x = 1"),
  issue(2, 13, "LL1_INSERT", "", " :)"),
  base
);


/* ':' debe convertirse en ':)' */

simulate(
  base.replace("} :)", "} :"),
  issue(3, 2, "LL1_REPLACE", ":", ":)"),
  base
);


/* ')' debe convertirse en '} :)' */

simulate(
  base.replace("} :)", ")"),
  issue(3, 0, "LL1_REPLACE", ")", "} :)"),
  base
);


/* Falta llave antes de :) */

simulate(
  base.replace("} :)", ":)"),
  issue(3, 0, "LL1_INSERT", "", "} "),
  base
);


/* Falta ')' en una expresión */

simulate(
  base.replace(
    "x = 1 :)",
    "x = (2+3 :)"
  ),
  issue(
    2,
    16,
    "LL1_INSERT",
    "",
    ")"
  ),
  base.replace(
    "x = 1 :)",
    "x = (2+3) :)"
  )
);


/*
 * Si el usuario cambió el código después de
 * recibir la sugerencia, no se aplica.
 */

const stale = base.replace(
  "entero x = 1",
  "entero y = 1"
);

simulate(
  stale,
  {
    ...issue(
      2,
      13,
      "LL1_INSERT",
      "",
      " :)"
    ),
    _source: base
  },
  stale,
  0
);


/* El frontend debe conservar las funciones actuales */

assert.ok(
  html.includes(
    "context: code.value.slice(0, word.start)"
  )
);

assert.ok(
  html.includes("Sugerencias en vivo")
);

assert.ok(
  html.includes('code.value = "";')
);


/*
 * El mecanismo heurístico antiguo no puede existir.
 * Las correcciones deben venir únicamente del LL(1).
 */

assert.ok(
  !html.includes("APPEND_BLOCK"),
  "APPEND_BLOCK todavía existe en el frontend"
);

console.log(
  `${checks} comprobaciones frontend LL(1) correctas.`
);
