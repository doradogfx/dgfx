// Menu music loop. The melody plays twice, the second time one octave higher. Renders two passes in non-real-time mode and keeps the second, so the reverb tail
// of the end wraps into the start and the loop has no seam.
// Run: sclang sounds/music.scd
(
var dir = thisProcess.nowExecutingPath.dirname;
var beat = 0.6; // 100 BPM
var loopBeats = 64; // 16 bars
var loopSec = loopBeats * beat;
var sampleRate = 44100;
var tmpPath = PathName.tmp +/+ "dgfx_music.wav";
var outPath = dir +/+ "menu_music.wav";
var chords = [[57, 60, 64], [53, 57, 60], [55, 60, 64], [55, 59, 62]]; // Am F C G
var roots = [45, 41, 48, 43];
var defs, song, score;

defs = [
    SynthDef(\pad, { |freq = 440, amp = 0.1, sustain = 1|
        var env = Env.linen(0.8, (sustain - 0.8).max(0.1), 1.5).ar(2);
        var sig = Saw.ar(freq * [0.995, 1.005]);
        sig = LPF.ar(sig, SinOsc.kr(0.1).range(700, 1600));
        Out.ar(0, sig * env * amp);
    }),
    SynthDef(\bass, { |freq = 110, amp = 0.2|
        var env = Env.perc(0.01, 0.4).ar(2);
        var sig = SinOsc.ar(freq) + LPF.ar(Saw.ar(freq), 400);
        Out.ar(0, (sig * env * amp) ! 2);
    }),
    SynthDef(\arp, { |freq = 880, amp = 0.05, pan = 0|
        var env = Env.perc(0.005, 0.25).ar(2);
        var sig = LPF.ar(Pulse.ar(freq, 0.3), 3000);
        Out.ar(0, Pan2.ar(sig * env * amp, pan));
    }),
    SynthDef(\lead, { |freq = 440, amp = 0.1, sustain = 1|
        var env = Env.linen(0.02, sustain, 0.3).ar(2);
        var vib = SinOsc.kr(5, 0, 0.004 * Line.kr(0, 1, 0.4)) + 1;
        var sig = LPF.ar(LFTri.ar(freq * vib) + (Pulse.ar(freq * vib, 0.4) * 0.2), 2500);
        Out.ar(0, (sig * env * amp) ! 2);
    }),
    SynthDef(\kick, { |amp = 0.4|
        var env = Env.perc(0.002, 0.3).ar(2);
        Out.ar(0, (SinOsc.ar(XLine.ar(120, 45, 0.1)) * env * amp) ! 2);
    }),
    SynthDef(\hat, { |amp = 0.05, pan = 0|
        var env = Env.perc(0.001, 0.05).ar(2);
        Out.ar(0, Pan2.ar(HPF.ar(WhiteNoise.ar, 7000) * env * amp, pan));
    }),
    SynthDef(\master, {
        var sig = In.ar(0, 2);
        sig = FreeVerb2.ar(sig[0], sig[1], 0.3, 0.8, 0.5);
        ReplaceOut.ar(0, Limiter.ar(sig, 0.8));
    })
];
defs.do(_.add);

song = Ppar([
    Pbind(\instrument, \pad, \midinote, Pseq(chords, 2), \dur, 8, \amp, 0.04),
    Pbind(\instrument, \bass, \midinote, Pseq(roots.collect { |r| Pseq([r], 16) }, 2), \dur, 0.5, \amp, 0.15),
    Pbind(\instrument, \arp,
        \midinote, Pseq(chords.collect { |c| Pseq(c ++ [c[0] + 12, c[2] + 12, c[0] + 12, c[2], c[1]] + 12, 4) }, 2),
        \dur, 0.25, \amp, 0.03, \pan, Pseq([-0.5, 0.5], inf)),
    Pbind(\instrument, \lead,
        \midinote, Pseq([0, 12].collect { |octave|
            Pseq([76, 74, 72, 71, 72, 69, 72, 74, 76, 77, 76, 79, 76, 72, 76, 74, 72, 71, 72, 74, 74] + octave)
        }),
        \dur, Pseq([1.5, 0.5, 1, 1, 2, 2, 1.5, 0.5, 1, 1, 4, 1.5, 0.5, 1, 1, 2, 2, 1.5, 0.5, 2, 4], 2),
        \legato, 0.9, \amp, 0.08),
    Pbind(\instrument, \kick, \dur, 2, \amp, 0.35, \note, Pn(0, loopBeats / 2)),
    Pbind(\instrument, \hat, \dur, Pseq([Rest(0.5), 0.5], loopBeats / 2), \amp, 0.04, \pan, 0.3)
]);

score = Pn(song, 2).asScore(loopSec * 2, protoEvent: (stretch: beat));
score.score = defs.collect { |d| [0.0, ['/d_recv', d.asBytes]] } ++ [
    [0.0, ['/s_new', \master, 2000, 1, 0]] // tail of root group, after all notes
] ++ score.score;
score.sort;

score.recordNRT(
    outputFilePath: tmpPath, headerFormat: "WAV", sampleFormat: "float", sampleRate: sampleRate,
    options: ServerOptions.new.numOutputBusChannels_(2), duration: loopSec * 2,
    action: {
        var frames = (loopSec * sampleRate).asInteger;
        var data = FloatArray.newClear(frames * 2);
        var inFile = SoundFile.openRead(tmpPath);
        var outFile = SoundFile.new.headerFormat_("WAV").sampleFormat_("int16").numChannels_(2).sampleRate_(sampleRate);
        inFile.seek(inFile.numFrames - frames);
        inFile.readData(data);
        inFile.close;
        outFile.openWrite(outPath);
        outFile.writeData(data);
        outFile.close;
        File.delete(tmpPath);
        ("rendered " ++ outPath).postln;
        0.exit;
    }
);
)
