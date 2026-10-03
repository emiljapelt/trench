// https://khendrikse.netlify.app/blog/send-data-with-formdata/#with-a-file-upload
const express = require('express');
const multer = require('multer');
const cookie_parser = require('cookie-parser')
const fs = require('node:fs');
const { execSync } = require('child_process');

const app = express();
const upload = multer();
app.use(cookie_parser());


const settings = JSON.parse(fs.readFileSync(process.argv[2]));

const trg_template = settings.trg;
const trenchc_path = settings.trenchc ?? '../trenchc';
const save_dir = settings.save_dir;

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
    const path = `${save_dir}/file_${counter++}.tr`;
    fs.writeFileSync(path, content);
    return path;
};

const try_make_team = (team, path) => {
    if (!(team in teams)) {
        teams[team] = {
            path: path,
            players: [],
        };
    }
   return teams[team];
};

const empty_file = `${save_dir}/empty.tr`;
if(!fs.existsSync(empty_file))
    fs.writeFileSync(empty_file, '// Empty :)');

const check = (team_sys, team, player) => {
    let temp_trg = `${save_dir}/temp.trg`;

    fs.copyFileSync(trg_template, temp_trg);
    
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

const sign = (path, body) => {
    let team = body.team ? `\n//Team: ${body.team}` : '';
    let player = body.player ? `\n//Player: ${body.player}` : '';
    fs.appendFileSync(path, `${team}${player}`);
};

const handler = (req, res) => {
    const { body, file } = req;

    if (!file) respond(res, 200, form(body.team, body.player, `<p>Please upload a file</p>`));
    let path = create_file(file.buffer);

    if (body.player) {
        console.log(`Player '${body.player}' submitted at ${(new Date()).toISOString()}`);

        let team = teams[body.team] ?? {};
        let result = check(team.sys, team.path, path);
        if (result.err) 
            fs.unlinkSync(path);
        else {
            if (body.player in players) {
                fs.unlinkSync(players[body.player]);
                fs.renameSync(path, players[body.player]);
                sign(players[body.player], body);
            } 
            else {
                console.log(`Player ${body.player} has file: ${path}`);
                players[body.player] = path;
                sign(players[body.player], body);
            }
        }

        respond(res, 200, form(body.team, body.player, `<pre>${result.stdout}</pre>`));
    }
    else if (body.team) {
        console.log(`Team '${body.team}' submitted at ${(new Date()).toISOString()}`);
        
        const team = try_make_team(body.team, path);
        let result = check(team.sys, team.path, null);
        if (result.err)
            fs.unlinkSync(path);
        else {
            console.log(`Team ${body.team} has file: ${path}`);
            sign(teams[body.team].path, body);
            respond(res, 200, form(body.team, null, `<pre>${result.stdout}</pre>`));
        }
    }
    else 
        respond(res, 200, form(body.team, body.player, `<p>Please upload a file</p>`));
};


app.get(/.*/, (req, res) => respond(res, 200, form()));
app.post('/filesubmit', upload.single('file'), handler);
const PORT = 8080;
app.listen(PORT, () => {
    console.log('Running...');
});