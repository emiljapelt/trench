// https://khendrikse.netlify.app/blog/send-data-with-formdata/#with-a-file-upload
const express = require('express');
const multer = require('multer');
const cookie_parser = require('cookie-parser')
const fs = require('node:fs');
const { execSync } = require('child_process');
const resolve = require('node:path').resolve;

const app = express();
const upload = multer();
app.use(cookie_parser());


const settings = JSON.parse(fs.readFileSync(process.argv[2]));

const trg_template = settings.trg;
const trenchc_path = settings.trenchc ?? '../trenchc';
const workdir = settings.workdir;
const savedir = resolve(workdir, 'save');
const empty_file = resolve(workdir, 'empty.tr');

const create_link = (link) => {
    switch (link.type ?? 'link') {
        case 'link': return `<a href=${link.path}>${link.name}</a>`;
        case 'download': return `<a download href=${link.path}>${link.name}</a>`;
    }
};
const links = settings.links.map(create_link).join('<br>');

let counter = 0;
const players = {};
const teams = {};
const team_sys = {};

const form = (team, player, append) => `
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
                    <label for="name">Team: </label>
                    <input class="form-elem" type="text" id="team" name="team" value="${team ?? ''}"/><br>
                </div>
                <div>
                    <label for="name">Player: </label>
                    <input class="form-elem" type="text" id="player" name="player" value="${player ?? ''}"/><br>
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
        <footer>
        ${links}
        </footer>
    </body>
`;

const respond = (res, code, content) => {
    res.writeHead(code, {'Content-Type': 'text/html'});
    res.write(content);
    res.send();
};

const create_file = (buffer) => {
    const content = Buffer.from(buffer).toString("utf-8");
    const path = resolve(savedir, `file_${counter++}.tr`);
    fs.writeFileSync(path, content);
    return path;
};


if(!fs.existsSync(empty_file))
    fs.writeFileSync(empty_file, '// Empty :)');

if(!fs.existsSync(savedir))
    fs.mkdirSync(savedir);


const check = (team_sys, team, player) => {
    let temp_trg = resolve(workdir, 'temp.trg');

    fs.copyFileSync(resolve(workdir, trg_template), temp_trg);
    
    team_sys = team_sys ? `system_library: "${team_sys}"`: '';
    team = team ? `library: "${team}"`: '';
    player ??= empty_file;

    fs.appendFileSync(temp_trg, `teams: [{name: "test" color: [0 0 0] origin: [0 0] ${team_sys} ${team} players: [{name: "test" files: ["${player}"]}]}]`);

    let result = {};

    try {
        execSync(`${trenchc_path} ${temp_trg}`, { cwd: '.'});
        result.stdout = 'Compiled';
        result.err = false;
    }
    catch (error) {
        result.stdout = error.stdout.toString();
        result.err = true;
    }

    fs.unlinkSync(temp_trg);

    return result;
};

const sign = (collection, target_type, target) => {
    fs.appendFileSync(collection[target], `\n//${target_type}: ${target}`);
};

const finalize_file = (check, path, target_type, target, collection) => {
    if (check.err) 
        fs.unlinkSync(path);
    else {
        console.log(`${target_type} '${target}' submitted at ${(new Date()).toISOString()}`);
        if (target in collection) {
            fs.copyFileSync(path, collection[target]);
            sign(collection, target_type, target);
            fs.unlinkSync(path);
        } 
        else {
            console.log(`${target_type} ${target} has file: ${path}`);
            collection[target] = path;
            sign(collection, target_type, target);
        }
    }
};

const handler = (req, res) => {
    const { body, file } = req;

    if (!file) respond(res, 200, form(body.team, body.player, `<p>Please upload a file</p>`));
    let path = create_file(file.buffer);

    if (body.player) {
        let result = check(team_sys[body.team], teams[body.team], path);
        finalize_file(result, path, 'Player', body.player, players);
        respond(res, 200, form(body.team, body.player, `<pre>${result.stdout}</pre>`));
    }
    else if (body.team) {   
        let result = check(team_sys[body.team], path, null);
        finalize_file(result, path, 'Team', body.team, teams);
        respond(res, 200, form(body.team, null, `<pre>${result.stdout}</pre>`));
    }
    else 
        respond(res, 200, form(body.team, body.player, `<p>Please upload a file</p>`));
};


if (settings.public)
    app.use(express.static(settings.public));
app.get(/.*/, (req, res) => respond(res, 200, form()));
app.post('/filesubmit', upload.single('file'), handler);
const PORT = 8080;
app.listen(PORT, () => {
    console.log('Running...');
});