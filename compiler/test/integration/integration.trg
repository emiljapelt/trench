mode: "inf";
actions: 1;
steps: 1000

map: [10 10]
themes: true
features: ["*"]


viewport: [10 10]

resources: {
    mana: 100
    wood: 100
    explosive: 100
    ammo: 100
    sapling: 100
    clay: 100
    bear_trap: 100
    metal: 100
}

teams: [{
    name: "team_1"
    color: [176 11 28]
    origin: [1 1]
    players: [{
        name: "player_1"
        files: ["./player.tr"]
    }]
    //library: "./team.tr"
    //system_library: "./team_sys.tr"
}]

//library: "./shared.tr"
system_library: "./sys.tr"
