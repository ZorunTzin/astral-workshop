import fs from 'node:fs/promises';
import {FileBlob, SpreadsheetFile} from '@oai/artifact-tool';
const repo='B:/Github/reimagined-workshop';
const wb=await SpreadsheetFile.importXlsx(await FileBlob.load(repo+'/tables/Runenwoerter-Schmieden-Entwurf.xlsx'));
const sheet=wb.worksheets.getItem('Runenwoerter');
console.log((await wb.inspect({kind:'region',sheetId:'Runenwoerter',range:'A7:B9',maxChars:1400})).ndjson);

const rows=sheet.getRange('A7:A226').values;
const lore=rows.findIndex(r=>String(r[0]).startsWith('Lore'))+7;
console.log('Lore',lore,rows[lore-7]);
await fs.mkdir(repo+'/build/spreadsheets/forge-labels',{recursive:true});
const preview=await wb.render({sheetName:'Runenwoerter',range:`A${lore}:E${lore}`,scale:1,format:'png'});
await fs.writeFile(repo+'/build/spreadsheets/forge-labels/before.png',new Uint8Array(await preview.arrayBuffer()));
if(process.argv.includes('--edit')){
 let changed=0;
 for(let i=0;i<rows.length;i++)if(typeof rows[i][0]==='string'&&rows[i][0].includes('Merc Equip')){sheet.getRange(`A${i+7}`).values=[[rows[i][0].replaceAll('Merc Equip','Helm')]];changed++;}
 wb.recalculate();
 const after=await wb.render({sheetName:'Runenwoerter',range:`A${lore}:E${lore}`,scale:1,format:'png'});
 await fs.writeFile(repo+'/build/spreadsheets/forge-labels/after.png',new Uint8Array(await after.arrayBuffer()));
 await fs.copyFile(repo+'/tables/Runenwoerter-Schmieden-Entwurf.xlsx',repo+'/build/spreadsheets/forge-labels/original.xlsx');
 await (await SpreadsheetFile.exportXlsx(wb)).save(repo+'/tables/Runenwoerter-Schmieden-Entwurf.xlsx');
 console.log('Corrected',changed,'helmet labels');
}
