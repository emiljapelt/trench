// https://khendrikse.netlify.app/blog/send-data-with-formdata/#with-a-file-upload
const express = require('express');
const multer = require('multer');
const fs = require('node:fs');
const { exec } = require('child_process');

const app = express();
const upload = multer();

const form = `
    <head>
        <style>
            div { margin: 0 0 10 10; }
        </style>
    </head>
    <body>
        <form action="filesubmit" method="post" enctype="multipart/form-data">
            <div>
                <label for="name">Name</label>
                <input type="text" id="name" name="name"/><br>
            </div>
            <div>
                <label for="file">File</label>
                <input type="file" id="file" name="file"/><br>
            </div>
            <div>
                <input type="submit" value="Submit"/>
            </div>
        </form>
    </body>
`;

let counter = 0;

app.get(/.*/, (req, res) => {
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.write(form);
    res.send();
});

app.post('/filesubmit', upload.single('file'), (req, res) => {
    const { body, file } = req;
    if (file && body.name) {
        const content = Buffer.from(file.buffer).toString("utf-8");
        const path = `./file_${counter++}.tr`;
        console.log(`${body.name} submitted ${path}`);
        fs.writeFileSync(path, content);
        exec(`../trenchc ${path}`, { cwd: '.'}, (err,stdout,stderr) => {
            res.writeHead(200, {'Content-Type': 'text/html'});
            res.write(`${form}<p>${stdout}</p>`);
            res.send();
        });
    }
    else {   
        res.writeHead(200, {'Content-Type': 'text/html'});
        res.write(`${form}<p>Please select a file AND write a name >:(</p>`);
        res.send();
    }
})

const PORT = 8080;
app.listen(PORT, () => {
    console.log('Running...');
});