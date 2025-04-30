// https://khendrikse.netlify.app/blog/send-data-with-formdata/#with-a-file-upload
const express = require('express');
const multer = require('multer');
const fs = require('node:fs');
const { exec } = require('child_process');

const app = express();
const upload = multer();

const form = `
    <head>
        <meta charset="UTF-8">
        <style>
            * {
                box-sizing: border-box
            } 
            div { 
                margin: 0 0 10 10; 
            }
            .main {
                display: flex;
                flex-direction: column;
                justify-content: center;
                align-items: center;
            }
            form {
                max-width: 20em;
            }
            .big-text {
                font-size: 5em;
            }
            .form-elem {
                width: 100%;
            }
        </style>
    </head>
    <body>
        <div class="main">
            <p class="big-text">🕳⛏🤖</p>
            <form action="filesubmit" method="post" enctype="multipart/form-data">
                <div>
                    <label for="name">Name: </label>
                    <input class="form-elem" type="text" id="name" name="name"/><br>
                </div>
                <div>
                    <label for="file">File: </label>
                    <input class="form-elem" type="file" id="file" name="file"/><br>
                </div>
                <div>
                    <input class="form-elem" type="submit" value="Submit"/>
                </div>
            </form>
        <div>
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
        fs.writeFileSync(path, content);
        exec(`../trenchc ${path}`, { cwd: '.'}, (err,stdout,stderr) => {
            if (err) fs.unlinkSync(path);
            else console.log(`${body.name} submitted ${path}`);
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