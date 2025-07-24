// https://khendrikse.netlify.app/blog/send-data-with-formdata/#with-a-file-upload
const express = require('express');
const multer = require('multer');
const cookie_parser = require('cookie-parser')
const fs = require('node:fs');
const { exec } = require('child_process');

const app = express();
const upload = multer();
app.use(cookie_parser());

const mode = process.argv[2];
const secret = process.argv[3];
let counter = 0;
let map = {};


const form = (name, append) => `
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
                    <input class="form-elem" type="text" id="name" name="name" value="${name ?? ''}"/><br>
                </div>
                <div>
                    <label for="file">File: </label>
                    <input class="form-elem" type="file" id="file" name="file"/><br>
                </div>
                <div>
                    <input class="form-elem" type="submit" value="Submit"/>
                </div>
            </form>
            ${append ?? ''}
        <div>
    </body>
`;

const respond = (res, code, content) => {
    res.writeHead(code, {'Content-Type': 'text/html'});
    res.write(content);
    res.send();
};

const modes = {
    /* Save nothing, just respond the result */
    checker: (req, res) => {
        const { body, file } = req;
        if (file && body.name) {
            const content = Buffer.from(file.buffer).toString("utf-8");
            const path = `./file_${counter++}.tr`;
            fs.writeFileSync(path, content);
            exec(`../trenchc ${path}`, { cwd: '.'}, (err,stdout,stderr) => {
                fs.unlinkSync(path);
                respond(res, 200, form(body.name, `<p>${stdout}</p>`));
            });
        }
        else {   
            respond(res, 200, form(body.name, `<p>Please select a file AND write a name >:(</p>`));
        }
    },

    /* Respond the result, and save each success to a new file */
    submit: (req, res) => {
        const { body, file } = req;
        if (file && body.name) {
            const content = Buffer.from(file.buffer).toString("utf-8");
            const path = `./file_${counter++}.tr`;
            fs.writeFileSync(path, content);
            exec(`../trenchc ${path}`, { cwd: '.'}, (err,stdout,stderr) => {
                if (err) fs.unlinkSync(path);
                else console.log(`${body.name} submitted ${path} at ${(new Date()).toISOString()}`);
                respond(res, 200, form(body.name, `<p>${stdout}</p>`));
            });
        }
        else {   
            respond(res, 200, form(body.name, `<p>Please select a file AND write a name >:(</p>`));
        }
    },

    /* Respond the result, and save success to players file overwriting previous */
    game: (req, res) => {
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
                respond(res, 200, form(body.name, `<p>${stdout}</p>`));
            });
        }
        else {   
            respond(res, 200, form(body.name, `<p>Please select a file AND write a name >:(</p>`));
        }
    },
};


if (mode in modes) {
    app.get(/.*/, (req, res) => respond(res, 200, form()));
    app.post('/filesubmit', upload.single('file'), modes[mode]);
    const PORT = 8080;
    app.listen(PORT, () => {
        console.log('Running...');
    });
}
else {
    console.log(`Unknown mode: ${mode}`);
}