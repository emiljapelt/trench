actions: 1
mode: inf
time_scale: 0
seed: 0

map: test.trm
viewport: 11,11

themes: *
features: *

auto_start: true
debug: false

resources: {
    mana: 10000
    wood: 1000
    explosive: 1000
    ammo: 1000
    sapling: 1000
    clay: 1000
}

team: {
    name: test_team
    color: 150,111,128
    origin: 6,6
    player: {
        name: tester
        files: test.tr
    }
}

settings: {
    projection: {
        upkeep: 0
        cost: 0
    }
}