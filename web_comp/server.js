// https://khendrikse.netlify.app/blog/send-data-with-formdata/#with-a-file-upload
const express = require('express');
const multer = require('multer');
const cookie_parser = require('cookie-parser')
const fs = require('node:fs');
const { exec } = require('child_process');

const app = express();
const upload = multer();
app.use(cookie_parser());

const setting = process.argv[2];
const secret = process.argv[3];
let counter = 0;
let map = {};


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

const respond = (res, code, content) => {
    res.writeHead(code, {'Content-Type': 'text/html'});
    res.write(content);
    res.send();
};

const checker = (req, res) => {
    const { body, file } = req;
    if (file && body.name) {
        const content = Buffer.from(file.buffer).toString("utf-8");
        const path = `./file_${counter++}.tr`;
        fs.writeFileSync(path, content);
        exec(`../trenchc ${path}`, { cwd: '.'}, (err,stdout,stderr) => {
            fs.unlinkSync(path);
            respond(res, 200, `${form}<p>${stdout}</p>`);
        });
    }
    else {   
        respond(res, 200, `${form}<p>Please select a file AND write a name >:(</p>`);
    }
};

const submit = (req, res) => {
    const { body, file } = req;
    if (file && body.name) {
        const content = Buffer.from(file.buffer).toString("utf-8");
        const path = `./file_${counter++}.tr`;
        fs.writeFileSync(path, content);
        exec(`../trenchc ${path}`, { cwd: '.'}, (err,stdout,stderr) => {
            if (err) fs.unlinkSync(path);
            else console.log(`${body.name} submitted ${path} at ${(new Date()).toISOString()}`);
            respond(res, 200, `${form}<p>${stdout}</p>`);
        });
    }
    else {   
        respond(res, 200, `${form}<p>Please select a file AND write a name >:(</p>`);
    }
};

const game = (req, res) => {
    const { body, file } = req;
    if (file && body.name) {
        const content = Buffer.from(file.buffer).toString("utf-8");
        const path = `./file_${counter++}.tr`;
        fs.writeFileSync(path, content);
        exec(`../trenchc ${path}`, { cwd: '.'}, (err,stdout,stderr) => {
            if (err) fs.unlinkSync(path);
            else {
                console.log(`${body.name} submitted at ${(new Date()).toISOString()}`);
                if (body.name in map) {
                    fs.writeFileSync(map[body.name], content);
                    fs.unlinkSync(path);
                } 
                else {
                    console.log(`${body.name} has file: ${path}`);
                    map[body.name] = path;
                }
            }
            respond(res, 200, `${form}<p>${stdout}</p>`);
        });
    }
    else {   
        respond(res, 200, `${form}<p>Please select a file AND write a name >:(</p>`);
    }

}

app.get(/.*/, (req, res) => {
    respond(res, 200, form);
});

switch (setting) {
    case 'checker':
        app.post('/filesubmit', upload.single('file'), checker);
        break;
    case 'submit':
        app.post('/filesubmit', upload.single('file'), submit);
        break;
    case 'game':
        app.post('/filesubmit', upload.single('file'), game);
        break;
    default:
        throw ("Unknown setting: " + setting) 
}

const PORT = 8080;
app.listen(PORT, () => {
    console.log('Running...');
});