// https://khendrikse.netlify.app/blog/send-data-with-formdata/#with-a-file-upload
const express = require('express');
const multer = require('multer');
const fs = require('node:fs');
const { exec } = require('child_process');

const app = express();
const upload = multer();

const form = `
    <form action="filecheck" method="post" enctype="multipart/form-data">
        <input type="file" name="file"/><br>
        <input type="submit" value="Check"/>
    </form>
`;

let counter = 0;

app.get('/', (req, res) => {
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.write(form);
    res.send();
});

app.post('/filecheck', upload.single('file'), (req, res) => {
    const { body, file } = req;
    if (file) {
        const content = Buffer.from(file.buffer).toString("utf-8");
        const path = `./file_${counter}.tr`;
        fs.writeFileSync(path, content);
        exec(`../trenchc ${path}`, { cwd: '.'}, (err,stdout,stderr) => {
            res.writeHead(200, {'Content-Type': 'text/html'});
            res.write(`${form}<p>${stdout}</p>`);
            res.send();
            fs.unlink(path, (err) => {});
        });
    }
    else {   
        res.writeHead(200, {'Content-Type': 'text/html'});
        res.write(form);
        res.send();
    }
})

const PORT = 8080;
app.listen(PORT, () => {
    console.log('Running...');
});