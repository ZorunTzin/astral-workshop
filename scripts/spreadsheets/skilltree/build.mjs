import fs from 'node:fs/promises';
import {Workbook, SpreadsheetFile} from '@oai/artifact-tool';

const generatedDir = new URL('../../../build/spreadsheets/skilltree/', import.meta.url);
const rows = JSON.parse(await fs.readFile(new URL('tree.json', generatedDir), 'utf8'));
const wb = Workbook.create();
const sheet = wb.worksheets.add('Skilltree');
sheet.showGridLines = false;
const last = 7 + rows.length;
sheet.getRange(`A1:J${last}`).format.font = {name:'Arial', size:10, color:'#202B38'};
sheet.getRange('A2').values = [['Astral Skilltree']];
sheet.getRange('A2').format.font = {name:'Arial', size:15, bold:true};
sheet.getRange('A3').values = [[`Workshop 0.9.8 | 10.11.2026 | ${rows.length} Cluster/Keystones, ${rows.reduce((n,r)=>n+(r[2]==='Keystone'?1:5),0)} Knoten`]];
sheet.getRange('A4').values = [['Gelbe Felder E–I bearbeiten. Jede Zelle enthält nur den Bonus dieses einzelnen Knotens.']];
sheet.getRange('A5').values = [['Keystones: nur Knoten 1. Maximal 3, freigeschaltet auf Level 30/60/90, je 1 Punkt.']];
sheet.getRange('A6').values = [['Änderungen speichern und die Datei zurückgeben; sie werden anschließend in die Mod übernommen.']];
sheet.getRange('A3:A6').format.font = {name:'Arial', size:10, color:'#596779'};
const headers = ['Region','Pfad','Position','Name','Knoten 1','Knoten 2','Knoten 3','Knoten 4','Knoten 5','Notizen'];
sheet.getRange('A7:J7').values = [headers];
sheet.getRange(`A8:J${last}`).values = rows;
sheet.getRange('A7:J7').format = {fill:'#243B55', font:{name:'Arial',size:10,bold:true,color:'#FFFFFF'},horizontalAlignment:'center',verticalAlignment:'center',rowHeight:28};
sheet.getRange(`A8:J${last}`).format.verticalAlignment = 'center';
sheet.getRange(`A8:J${last}`).format.rowHeight = 76;
sheet.getRange(`A8:D${last}`).format.fill = '#F0F3F7';
sheet.getRange(`E8:I${last}`).format.fill = '#FFF3CD';
sheet.getRange(`D8:J${last}`).format.wrapText = true;
sheet.getRange(`B8:B${last}`).setNumberFormat('0');
const widths = {A:17,B:7,C:13,D:22,E:27,F:37,G:37,H:37,I:45,J:29};
for (const [col,width] of Object.entries(widths)) sheet.getRange(`${col}1:${col}${last}`).format.columnWidth = width;
for (let n=0;n<rows.length;n++) {
  const r=8+n;
  if(rows[n][2]==='Keystone') sheet.getRange(`F${r}:I${r}`).format.fill='#F0F3F7';
  if(n===0||rows[n][0]!==rows[n-1][0]) sheet.getRange(`A${r}:J${r}`).format.borders = {top:{style:'medium',color:'#8798AB'}};
}
sheet.tables.add(`A7:J${last}`,true,'AstralSkilltree');
sheet.freezePanes.freezeRows(7);
sheet.freezePanes.freezeColumns(4);
wb.recalculate();
console.log((await wb.inspect({kind:'table',range:`Skilltree!D${last}:I${last}`,include:'values',tableMaxRows:1,tableMaxCols:6,maxChars:1800})).ndjson);
console.log((await wb.inspect({kind:'match',searchTerm:'#REF!|#DIV/0!|#VALUE!|#NAME\\?|#NUM!',options:{useRegex:true,maxResults:5},maxChars:500})).ndjson);
for(const [name,range] of [['top','A2:J10'],['swarm',`A${last-3}:J${last}`]]) {
  const preview=await wb.render({sheetName:'Skilltree',range,scale:1,format:'png'});
  await fs.writeFile(new URL(`preview-${name}.png`,generatedDir),new Uint8Array(await preview.arrayBuffer()));
}
const file=await SpreadsheetFile.exportXlsx(wb);
await file.save(decodeURIComponent(new URL('../../../tables/Astral-Skilltree-0.9.5.xlsx',import.meta.url).pathname.replace(/^\/([A-Za-z]:)/,'$1')));
console.log('Exported editable skilltree workbook with all active records.');
