/* Verifica que los códigos nuevos LL1_INSERT y LL1_REPLACE se apliquen
 * sin modificar el cascarón del editor. */
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const path=require('node:path');
const html=fs.readFileSync(path.join(__dirname,'..','index.html'),'utf8');
const start=html.indexOf('function applySuggestion(s){');
const end=html.indexOf('\nasync function checkSyntax(',start);
assert.ok(start>0&&end>start);
const js=html.slice(start,end);
let n=0;
function simulate(input,issue,expected,expectedRuns=1){
    const code={value:input};let runs=0;
    const ctx={code,$:()=>({textContent:''}),msg:()=>{},clearTimeout:()=>{},
      liveReviewTimer:null,syntaxSequence:0,closeCompletions:()=>{},
      renderIssues:()=>{},jumpToLine:()=>{},checkSyntax:()=>runs++};
    vm.runInNewContext(js,ctx);ctx.applySuggestion(issue);
    assert.equal(code.value,expected);
    assert.equal(runs,expectedRuns);n++;
}
const base='defina vacio principal() {\n entero x = 1 :)\n} :)\n';
const issue=(line,column,code,expected,replacement)=>({line,column,code,expected,replacement});
simulate(base.replace('x = 1 :)','x = 1'),issue(2,13,'LL1_INSERT','',' :)'),base);
simulate(base.replace('} :)','} :'),issue(3,2,'LL1_REPLACE',':',':)'),base);
simulate(base.replace('} :)',')'),issue(3,0,'LL1_REPLACE',')','} :)'),base);
simulate(base.replace('} :)',':)'),issue(3,0,'LL1_INSERT','','} '),base);
simulate(base.replace('x = 1 :)','x = (2+3 :)'),issue(2,16,'LL1_INSERT','',')'),base.replace('x = 1 :)','x = (2+3) :)'));
const stale=base.replace('entero x = 1','entero y = 1');
simulate(stale,{...issue(2,13,'LL1_INSERT','',' :)'),_source:base},stale,0);
assert.ok(html.includes('context:code.value.slice(0,word.start)'));
assert.ok(html.includes('Sugerencias en vivo'));
assert.ok(html.includes("code.value='';code.defaultValue='';"));
assert.ok(!html.includes("s.code==='APPEND_BLOCK'"));
console.log(`${n} comprobaciones frontend LL(1) correctas.`);
