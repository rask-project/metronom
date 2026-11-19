// Metronom site: download links, copy buttons, the Features menu
// and the beat explorer (with its player) on How to use.

(function () {
    "use strict";

    // A JSON block the page renders from _data/i18n (or {} when it is missing).
    function readJson(element) {
        try {
            return JSON.parse(element ? element.textContent : "{}") || {};
        } catch (error) {
            return {};
        }
    }

    var STRINGS = readJson(document.getElementById("site-strings"));

    function format(template, values) {
        return String(template || "").replace(/\{(\w+)\}/g, function (match, key) {
            return key in values ? values[key] : match;
        });
    }

    // Point each Download button at its file in the latest release (the
    // data-download attribute is a pattern for the file name) and show the
    // version. Without the API (offline, rate limit) they keep linking to the
    // Releases page.
    function resolveDownload() {
        var links = document.querySelectorAll("[data-download]");
        var versions = document.querySelectorAll("[data-version]");
        if (!links.length && !versions.length) {
            return;
        }
        fetch("https://api.github.com/repos/rask-project/metronom/releases/latest", {
            headers: { Accept: "application/vnd.github+json" }
        })
            .then(function (response) {
                return response.ok ? response.json() : null;
            })
            .then(function (release) {
                if (!release) {
                    return;
                }
                var assets = release.assets || [];
                var version = String(release.tag_name || "").replace(/^v/, "");
                links.forEach(function (link) {
                    var pattern = new RegExp(link.getAttribute("data-download") || "\\.dmg$", "i");
                    var asset = assets.find(function (candidate) {
                        return pattern.test(candidate.name);
                    });
                    if (asset) {
                        link.href = asset.browser_download_url;
                    }
                });
                versions.forEach(function (node) {
                    if (version) {
                        node.textContent = node.getAttribute("data-version").replace("%v", version);
                        node.hidden = false;
                    }
                });
            })
            .catch(function () {});
    }

    // A Copy button on every code block marked .code-copy.
    function addCopyButtons() {
        document.querySelectorAll(".code-copy").forEach(function (block) {
            var code = block.querySelector("code");
            if (!code || !navigator.clipboard) {
                return;
            }
            var button = document.createElement("button");
            button.type = "button";
            button.textContent = STRINGS.copy || "Copy";
            button.addEventListener("click", function () {
                navigator.clipboard.writeText(code.textContent.trim()).then(function () {
                    button.textContent = STRINGS.copied || "Copied";
                    setTimeout(function () {
                        button.textContent = STRINGS.copy || "Copy";
                    }, 1600);
                });
            });
            block.appendChild(button);
        });
    }

    // Highlight the Features menu item of the section in view.
    function trackFeaturesMenu() {
        var links = Array.prototype.slice.call(document.querySelectorAll(".features-menu a[href^='#']"));
        if (!links.length || !("IntersectionObserver" in window)) {
            return;
        }
        var byId = {};
        links.forEach(function (link) {
            byId[link.getAttribute("href").slice(1)] = link;
        });
        var observer = new IntersectionObserver(function (entries) {
            entries.forEach(function (entry) {
                if (!entry.isIntersecting) {
                    return;
                }
                links.forEach(function (link) {
                    link.classList.remove("is-current");
                });
                var current = byId[entry.target.id];
                if (current) {
                    current.classList.add("is-current");
                    // The menu is a horizontal strip on narrow screens:
                    // keep the item in view without scrolling the page.
                    var list = current.closest("ul");
                    var offset = current.getBoundingClientRect().left - list.getBoundingClientRect().left;
                    list.scrollLeft += offset - (list.clientWidth - current.offsetWidth) / 2;
                }
            });
        }, { rootMargin: "-30% 0px -60% 0px" });
        Object.keys(byId).forEach(function (id) {
            var section = document.getElementById(id);
            if (section) {
                observer.observe(section);
            }
        });
    }

    // The beat explorer: the same rules as Metronome.cpp. Lengths are in
    // thirty-second notes, so a dotted quarter is a whole number.
    var NOTES = [
        { key: "half", icon: "half-note", length: 16 },
        { key: "dq", icon: "dotted-quarter-note", length: 12 },
        { key: "quarter", icon: "quarter-note", length: 8 },
        { key: "eighth", icon: "eighth-note", length: 4 },
        { key: "sixteenth", icon: "sixteenth-note", length: 2 },
        { key: "thirtysecond", icon: "thirty-second-note", length: 1 }
    ];
    var MAX_SUBDIVISIONS = 8;

    // The app's three clicks (kAccentClick, kBeatClick, kSubdivisionClick).
    var CLICKS = [
        { freq: 3400, duration: 0.012, decay: 0.0025, amplitude: 0.9 },
        { freq: 2200, duration: 0.009, decay: 0.002, amplitude: 0.6 },
        { freq: 1600, duration: 0.006, decay: 0.0015, amplitude: 0.3 }
    ];
    var LOOKAHEAD = 0.12;
    var SCHEDULE_MS = 25;

    function meter(numerator, denominator) {
        var compound = denominator === 8 && numerator > 3 && numerator % 3 === 0;
        return {
            compound: compound,
            beats: compound ? numerator / 3 : numerator,
            beatLength: (32 / denominator) * (compound ? 3 : 1)
        };
    }

    function notesPerBeat(beatLength, note) {
        if (note.length > beatLength || beatLength % note.length !== 0) {
            return 0;
        }
        var count = beatLength / note.length;
        return count <= MAX_SUBDIVISIONS ? count : 0;
    }

    function plural(count, words) {
        words = words || ["", ""];
        return count + " " + (count === 1 ? words[0] : words[1]);
    }

    // A click rendered like renderClick() in Metronome.cpp: a decaying sine with a little noise.
    function renderClick(context, sound) {
        var rate = context.sampleRate;
        var length = Math.floor(rate * sound.duration);
        var buffer = context.createBuffer(1, length, rate);
        var data = buffer.getChannelData(0);
        var seed = 1;
        for (var i = 0; i < length; i++) {
            var time = i / rate;
            seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
            var noise = (seed >>> 8) / 8388608 - 1;
            var tone = Math.sin(2 * Math.PI * sound.freq * time);
            data[i] = (0.6 * tone + 0.4 * noise) * Math.exp(-time / sound.decay) * sound.amplitude;
        }
        return buffer;
    }

    function initExplorer() {
        var root = document.getElementById("beat-explorer");
        if (!root) {
            return;
        }
        var numerator = root.querySelector("[data-numerator]");
        var denominator = root.querySelector("[data-denominator]");
        var bpm = root.querySelector("[data-bpm]");
        var notesBox = root.querySelector("[data-notes]");
        var result = root.querySelector("[data-result]");
        var bar = root.querySelector("[data-bar]");
        var playButton = root.querySelector("[data-play]");
        var playLabel = root.querySelector("[data-play-label]");
        var labels = readJson(root.querySelector("[data-labels]"));
        var names = readJson(root.querySelector("[data-names]"));
        var locale = document.documentElement.lang || "en";
        var iconBase = document.querySelector("link[rel=stylesheet]").href.replace(/css\/site\.css.*$/, "icons/");
        var selected = "quarter";

        // The player: clicks are queued on the audio clock a little ahead (LOOKAHEAD),
        // and the beat bar lights each one when its time comes.
        var player = { context: null, buffers: null, timer: 0, playing: false, queue: [], shown: "", lit: null };

        for (var n = 1; n <= 12; n++) {
            var option = document.createElement("option");
            option.textContent = n;
            option.selected = n === 4;
            numerator.appendChild(option);
        }

        // The bar the controls describe now.
        function current() {
            var m = meter(Number(numerator.value), Number(denominator.value));
            var available = NOTES.filter(function (note) {
                return notesPerBeat(m.beatLength, note) > 0;
            });
            var note = available.find(function (candidate) {
                return candidate.key === selected;
            }) || available[0];
            selected = note.key;
            var perBeat = notesPerBeat(m.beatLength, note);
            return {
                meter: m,
                available: available,
                note: note,
                signature: numerator.value + "/" + denominator.value,
                beats: m.beats,
                perBeat: perBeat,
                bpm: Math.min(300, Math.max(40, Number(bpm.value) || 120)),
                id: numerator.value + "/" + denominator.value + ":" + perBeat
            };
        }

        function drawBar(config) {
            bar.textContent = "";
            bar.setAttribute("aria-label", format(STRINGS.bar_label, {
                sig: config.signature,
                note: names[config.note.key] || config.note.key,
                beats: config.beats,
                perBeat: config.perBeat
            }));
            for (var b = 0; b < config.beats; b++) {
                var beat = document.createElement("span");
                beat.className = "beat";
                for (var s = 0; s < config.perBeat; s++) {
                    var cell = document.createElement("i");
                    cell.className = "note";
                    beat.appendChild(cell);
                }
                bar.appendChild(beat);
            }
            player.shown = config.id;
            player.lit = null;
        }

        function render() {
            var config = current();
            var beatNote = NOTES.find(function (note) {
                return note.length === config.meter.beatLength;
            });
            var perMinute = config.bpm * config.perBeat;

            notesBox.textContent = "";
            config.available.forEach(function (note) {
                var button = document.createElement("button");
                button.type = "button";
                button.setAttribute("aria-pressed", String(note === config.note));
                var glyph = document.createElement("span");
                glyph.className = "glyph";
                glyph.style.setProperty("--glyph", "url('" + iconBase + note.icon + ".svg')");
                button.appendChild(glyph);
                var text = (labels[note.key] || note.key) + " · " +
                    format(STRINGS.per_beat, { n: notesPerBeat(config.meter.beatLength, note) });
                button.appendChild(document.createTextNode(text));
                button.addEventListener("click", function () {
                    selected = note.key;
                    render();
                });
                notesBox.appendChild(button);
            });

            result.innerHTML = format(STRINGS.result, {
                sig: config.signature,
                beats: plural(config.beats, STRINGS.beats),
                beat: names[beatNote.key] || beatNote.key,
                compound: config.meter.compound ? STRINGS.compound : "",
                label: labels[config.note.key] || config.note.key,
                perBeat: config.perBeat,
                clicks: plural(config.beats * config.perBeat, STRINGS.clicks),
                bpm: config.bpm,
                perMinute: perMinute.toLocaleString(locale),
                perSecond: (perMinute / 60).toLocaleString(locale, { maximumFractionDigits: 1 })
            });

            // While playing, the bar changes on the next beat, with the sound.
            if (!player.playing) {
                drawBar(config);
            }
        }

        function schedule() {
            var context = player.context;
            while (player.next < context.currentTime + LOOKAHEAD) {
                if (player.sub === 0) {
                    player.config = current();
                    if (player.beat >= player.config.beats) {
                        player.beat = 0;
                    }
                }
                var config = player.config;
                var click = player.sub > 0 ? 2 : player.beat === 0 ? 0 : 1;
                var source = context.createBufferSource();
                source.buffer = player.buffers[click];
                source.connect(context.destination);
                source.start(player.next);
                player.queue.push({ time: player.next, beat: player.beat, sub: player.sub, config: config });

                player.next += 60 / config.bpm / config.perBeat;
                player.sub += 1;
                if (player.sub >= config.perBeat) {
                    player.sub = 0;
                    player.beat = (player.beat + 1) % config.beats;
                }
            }
        }

        function draw() {
            if (!player.playing) {
                return;
            }
            var now = player.context.currentTime;
            while (player.queue.length && player.queue[0].time <= now) {
                var tick = player.queue.shift();
                if (tick.config.id !== player.shown) {
                    drawBar(tick.config);
                }
                var beat = bar.children[tick.beat];
                var cell = beat && beat.children[tick.sub];
                if (player.lit) {
                    player.lit.classList.remove("is-on");
                }
                if (cell) {
                    cell.classList.add("is-on");
                }
                player.lit = cell;
            }
            requestAnimationFrame(draw);
        }

        function setPlaying(playing) {
            player.playing = playing;
            root.classList.toggle("is-playing", playing);
            bar.classList.toggle("is-playing", playing);
            playButton.setAttribute("aria-pressed", String(playing));
            playLabel.textContent = playing ? STRINGS.stop : STRINGS.play;
        }

        function start() {
            if (!player.context) {
                var AudioContext = window.AudioContext || window.webkitAudioContext;
                player.context = new AudioContext();
                player.buffers = CLICKS.map(function (sound) {
                    return renderClick(player.context, sound);
                });
            }
            player.context.resume();
            player.beat = 0;
            player.sub = 0;
            player.queue = [];
            player.next = player.context.currentTime + 0.06;
            drawBar(current());
            setPlaying(true);
            schedule();
            player.timer = setInterval(schedule, SCHEDULE_MS);
            requestAnimationFrame(draw);
        }

        function stop() {
            clearInterval(player.timer);
            setPlaying(false);
            player.queue = [];
            drawBar(current());
        }

        if (window.AudioContext || window.webkitAudioContext) {
            playButton.addEventListener("click", function () {
                if (player.playing) {
                    stop();
                } else {
                    start();
                }
            });
            // Background tabs throttle timers, which would break the beat.
            document.addEventListener("visibilitychange", function () {
                if (document.hidden && player.playing) {
                    stop();
                }
            });
        } else {
            playButton.disabled = true;
            playButton.title = STRINGS.no_audio;
        }

        [numerator, denominator, bpm].forEach(function (input) {
            input.addEventListener("input", render);
        });
        render();
        root.hidden = false;
    }

    document.addEventListener("DOMContentLoaded", function () {
        resolveDownload();
        addCopyButtons();
        trackFeaturesMenu();
        initExplorer();
    });
})();
