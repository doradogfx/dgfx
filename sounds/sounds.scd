// Game sound effects. Renders each SynthDef to sounds/<name>.wav in non-real-time mode (no audio device needed).
// Run: sclang sounds/sounds.scd
(
var dir = thisProcess.nowExecutingPath.dirname;
var defs = [
    // name, duration (s), SynthDef
    [\shoot, 0.15, SynthDef(\shoot, {
        var env = Env.perc(0.001, 0.12).ar(2);
        var freq = XLine.ar(1400, 300, 0.12);
        var sig = Pulse.ar(freq, 0.3) * 0.5 + SinOsc.ar(freq * 0.5);
        Out.ar(0, LPF.ar(sig, 5000) * env * 0.4);
    })],
    [\hit, 0.1, SynthDef(\hit, {
        var env = Env.perc(0.001, 0.07).ar(2);
        var thump = SinOsc.ar(XLine.ar(220, 80, 0.07));
        var crack = BPF.ar(WhiteNoise.ar, 2500, 0.8) * Env.perc(0.001, 0.03).ar;
        Out.ar(0, (thump + (crack * 2)) * env * 0.5);
    })],
    [\enemy_death, 0.45, SynthDef(\enemy_death, {
        var env = Env.perc(0.005, 0.4, curve: -3).ar(2);
        var noise = RLPF.ar(WhiteNoise.ar, XLine.ar(4000, 200, 0.4), 0.3);
        var tone = LFPulse.ar(XLine.ar(600, 60, 0.35)) - 0.5;
        Out.ar(0, (noise * 0.6 + (tone * 0.4)) * env * 0.5);
    })],
    [\xp_pickup, 0.15, SynthDef(\xp_pickup, {
        var env = Env.perc(0.001, 0.12).ar(2);
        var freq = Duty.ar(0.04, 0, Dseq([1320, 1760], 1), 0, 1760);
        Out.ar(0, SinOsc.ar(freq) * env * 0.3);
    })],
    [\level_up, 0.8, SynthDef(\level_up, {
        var env = Env.linen(0.005, 0.55, 0.2).ar(2);
        var freq = Duty.ar(0.09, 0, Dseq([60, 64, 67, 72, 76, 79].midicps, 1), 0, 84.midicps);
        var sig = Pulse.ar(freq, 0.25) * 0.3 + SinOsc.ar(freq * 2, 0, 0.2);
        Out.ar(0, LPF.ar(sig, 6000) * env * 0.5);
    })],
    [\player_hurt, 0.3, SynthDef(\player_hurt, {
        var env = Env.perc(0.002, 0.25).ar(2);
        var sig = Saw.ar(XLine.ar(300, 90, 0.25) * [1, 1.02]).sum;
        Out.ar(0, (sig * 3).tanh * env * 0.3);
    })],
    [\jump, 0.18, SynthDef(\jump, {
        var env = Env.perc(0.005, 0.15).ar(2);
        var freq = XLine.ar(200, 700, 0.15);
        Out.ar(0, (SinOsc.ar(freq) + (Pulse.ar(freq, 0.5) * 0.2)) * env * 0.35);
    })],
    [\boss_spawn, 3.0, SynthDef(\boss_spawn, {
        var env = Env([0, 1, 1, 0], [0.8, 1.2, 1.0], [2, 0, -3]).ar(2);
        var rumble = LPF.ar(BrownNoise.ar, 120) * 2;
        var growl = LPF.ar(Saw.ar([41, 41.5, 55]).sum, XLine.ar(200, 900, 2.0));
        var sig = (rumble + (growl * 0.5)) * (1 + (SinOsc.ar(6) * 0.3));
        Out.ar(0, (sig * 1.5).tanh * env * 0.5);
    })],
    [\win, 1.6, SynthDef(\win, {
        var env = Env.linen(0.005, 1.1, 0.45).ar(2);
        var freq = Duty.ar(Dseq([0.12, 0.12, 0.12, 1], 1), 0, Dseq([67, 72, 76, 84].midicps, 1), 0);
        var sig = Pulse.ar(freq * [1, 1.005], 0.3).sum * 0.25 + SinOsc.ar(freq * 0.5, 0, 0.3);
        Out.ar(0, LPF.ar(sig, 5000) * env * 0.45);
    })],
    [\game_over, 1.8, SynthDef(\game_over, {
        var env = Env.linen(0.005, 1.2, 0.55).ar(2);
        var freq = Duty.ar(Dseq([0.3, 0.3, 0.3, 1], 1), 0, Dseq([67, 63, 60, 55].midicps, 1), 0);
        var sig = Saw.ar(freq * [1, 1.01]).sum;
        Out.ar(0, LPF.ar(sig, 1500) * env * 0.25);
    })],
    [\ui_select, 0.06, SynthDef(\ui_select, {
        var env = Env.perc(0.001, 0.045).ar(2);
        Out.ar(0, SinOsc.ar(XLine.ar(1800, 1200, 0.04)) * env * 0.3);
    })]
];
var remaining = defs.size;
var options = ServerOptions.new.numOutputBusChannels_(1);

defs.do { |entry|
    var name = entry[0], dur = entry[1], def = entry[2];
    var score = Score([
        [0.0, ['/d_recv', def.asBytes]],
        [0.0, Synth.basicNew(name, Server.default, 1000).newMsg]
    ]);
    score.recordNRT(
        outputFilePath: dir +/+ name ++ ".wav",
        headerFormat: "WAV", sampleFormat: "int16", sampleRate: 44100,
        options: options, duration: dur,
        action: {
            ("rendered " ++ name).postln;
            remaining = remaining - 1;
            if (remaining == 0) { 0.exit };
        }
    );
};
)
