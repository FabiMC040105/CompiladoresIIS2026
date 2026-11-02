/* Pruebas del algoritmo de aplicación en la interfaz, sin navegador. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const path = require('node:path');
const html = fs.readFileSync(path.join(__dirname,'..','index.html'),'utf8');
const start = html.indexOf('function applySuggestion(s){');
const end = html.indexOf('\nasync function checkSyntax(',start);
assert.ok(start>0 && end>start);
const source=html.slice(start,end);
let cases=0;
function simulate(input,issue,expected){
  const code={value:input};let message='';let rechecked=0;
  const elements={result:{textContent:''}};
  const context={code,$:id=>elements[id]||{textContent:''},
    msg:t=>{message=t},clearTimeout:()=>{},liveReviewTimer:null,syntaxSequence:0,
    closeCompletions:()=>{},renderIssues:()=>{},jumpToLine:()=>{},
    checkSyntax:()=>{rechecked++}};
  vm.runInNewContext(source,context);
  context.applySuggestion(issue);
  assert.equal(code.value,expected,`Fallo en ${issue.code}: ${message}`);
  assert.equal(rechecked,input===expected?0:1);
  cases++;
}
const base='defina vacio principal() {\n  entero x = 1 :)\n} :)\n';
const issue=(line,column,code,expected,replacement)=>(
 {line,column,code,expected,replacement});
simulate(base.replace('} :)',')'),issue(3,0,'REPLACE_ORPHAN_RPAREN_WITH_BLOCK',')','} :)'),base);
simulate(base.replace('} :)',':'),issue(3,0,'REPLACE_ORPHAN_COLON_WITH_BLOCK',':','} :)'),base);
simulate(base.replace('} :)',':)'),issue(3,0,'INSERT_RBRACE_BEFORE_FIN','','} '),base);
simulate(base.replace('} :)','} )'),issue(3,2,'INSERT_COLON_BEFORE_RPAREN','',':'),base);
simulate(base.replace('} :)','} :'),issue(3,3,'INSERT_RPAREN_AFTER_COLON','',')'),base);
simulate(base.replace('} :)','}'),issue(3,1,'APPEND_FIN_LINEA','',' :)'),base);
simulate(base.replace('entero x = 1 :)','entero x = 1 :'),issue(2,16,'INSERT_RPAREN_AFTER_COLON','',')'),base);
simulate(base.replace('entero x = 1 :)','entero x = 1 )'),issue(2,15,'INSERT_COLON_BEFORE_RPAREN','',':'),base);
simulate(base.replace('entero x = 1 :)','entero x = 1'),issue(2,14,'APPEND_FIN_LINEA','',' :)'),base);
simulate('defina vacio principal() {\n entero x = 1 :)\n',issue(2,17,'APPEND_BLOCK','',''),
 'defina vacio principal() {\n entero x = 1 :)\n} :)\n');
// Una corrección obsoleta no debe aplicarse aunque coincida su columna.
const stale=base.replace('entero x = 1','entero y = 1');
simulate(stale,{...issue(2,14,'APPEND_FIN_LINEA','',' :)'),_source:base},stale);
assert.ok(html.includes('Sugerencias en vivo'));
assert.ok(html.includes('scheduleLiveReview()'));
assert.ok(html.includes("code.value='';code.defaultValue='';"));
console.log(`${cases} pruebas de aplicación JS aprobadas; revisión en vivo y editor vacío presentes.`);
