mode: "inf";
actions: 1;
steps: 1000
time_scale: 1

map: "../maps/islands.trm"

themes: ["military" "forestry" "pottery"]
features: ["*"]

auto_start: true
auto_resize: true

feed_width: 32
viewport: [76 22]

resources: {
    mana: 100
    wood: 100
    explosive: 0
    ammo: 1000
    sapling: 1000
    clay: 0
    bear_trap: 0
    metal: 0
}

teams: [{
    name: "team_1"
    color: [176 11 28]
    origin: [25 8]
    players: [{
        name: "player_1"
        files: ["example.tr"]
    }]
    library: "example_lib.tr"
}]