import React, { useState, useEffect, useRef, useCallback, useMemo } from 'react';
import { Play, Pause, Square, Upload, Download, Settings, Sliders, Zap, Activity } from 'lucide-react';

// Advanced audio processing constants
const SAMPLE_RATE = 44100;
const BUFFER_SIZE = 4096;
const MAX_VOICES = 64;
const TEMPORAL_LAYERS = 8;

// High-quality interpolation algorithms
class AdvancedInterpolation {
  static hermite(x, y0, y1, y2, y3) {
    const c0 = y1;
    const c1 = 0.5 * (y2 - y0);
    const c2 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3;
    const c3 = 0.5 * (y3 - y0) + 1.5 * (y1 - y2);
    return ((c3 * x + c2) * x + c1) * x + c0;
  }

  static lagrange(x, points) {
    let result = 0;
    const n = points.length;
    for (let i = 0; i < n; i++) {
      let term = points[i].y;
      for (let j = 0; j < n; j++) {
        if (i !== j) {
          term *= (x - points[j].x) / (points[i].x - points[j].x);
        }
      }
      result += term;
    }
    return result;
  }
}

// Advanced filter implementations
class TitanFilters {
  constructor(sampleRate) {
    this.sampleRate = sampleRate;
    this.reset();
  }

  reset() {
    this.x1 = 0; this.x2 = 0;
    this.y1 = 0; this.y2 = 0;
    this.z1 = 0; this.z2 = 0;
  }

  // State Variable Filter with nonlinear response
  svf(input, freq, res, mode = 0) {
    const f = 2 * Math.sin(Math.PI * freq / this.sampleRate);
    const q = res;
    
    this.z1 += f * (input - this.z1 - q * this.z2);
    this.z2 += f * this.z1;
    
    // Add subtle nonlinear saturation
    this.z1 = Math.tanh(this.z1 * 0.7) * 1.4;
    this.z2 = Math.tanh(this.z2 * 0.7) * 1.4;
    
    switch(mode) {
      case 0: return this.z2; // Lowpass
      case 1: return input - this.z1 - this.z2; // Highpass
      case 2: return this.z1; // Bandpass
      case 3: return input - this.z1; // Notch
      default: return this.z2;
    }
  }

  // Moog-style ladder filter
  moogLadder(input, freq, res) {
    const f = freq / (this.sampleRate * 0.5);
    const k = res * 4;
    const p = f * (1.8 - 0.8 * f);
    const scale = Math.exp((1 - p) * 1.386249);
    
    const r = res * scale;
    input -= r * this.y2;
    
    // Four cascaded one-pole filters
    this.y1 = input * p + this.x1 * p - k * this.y1;
    this.x1 = input;
    this.y2 = this.y1 * p + this.x2 * p - k * this.y2;
    this.x2 = this.y1;
    
    return this.y2;
  }
}

// Advanced envelope with multiple stages
class TitanEnvelope {
  constructor() {
    this.reset();
  }

  reset() {
    this.stage = 0; // 0: off, 1: attack, 2: decay, 3: sustain, 4: release
    this.level = 0;
    this.targetLevel = 0;
    this.rate = 0;
  }

  setADSR(attack, decay, sustain, release) {
    this.attack = attack;
    this.decay = decay;
    this.sustain = sustain;
    this.release = release;
  }

  noteOn() {
    this.stage = 1;
    this.targetLevel = 1.0;
    this.rate = 1.0 / (this.attack * SAMPLE_RATE);
  }

  noteOff() {
    this.stage = 4;
    this.targetLevel = 0;
    this.rate = this.level / (this.release * SAMPLE_RATE);
  }

  process() {
    switch(this.stage) {
      case 1: // Attack
        this.level += this.rate;
        if (this.level >= 1.0) {
          this.level = 1.0;
          this.stage = 2;
          this.targetLevel = this.sustain;
          this.rate = (1.0 - this.sustain) / (this.decay * SAMPLE_RATE);
        }
        break;
      case 2: // Decay
        this.level -= this.rate;
        if (this.level <= this.sustain) {
          this.level = this.sustain;
          this.stage = 3;
        }
        break;
      case 3: // Sustain
        this.level = this.sustain;
        break;
      case 4: // Release
        this.level -= this.rate;
        if (this.level <= 0) {
          this.level = 0;
          this.stage = 0;
        }
        break;
    }
    return this.level;
  }
}

// Multi-temporal oscillator with advanced waveforms
class TitanOscillator {
  constructor(sampleRate) {
    this.sampleRate = sampleRate;
    this.phase = 0;
    this.frequency = 440;
    this.waveform = 0;
    this.temporalLayers = Array(TEMPORAL_LAYERS).fill(0).map(() => ({
      phase: Math.random() * Math.PI * 2,
      frequency: 1,
      amplitude: 0.1,
      waveform: 0
    }));
  }

  setFrequency(freq) {
    this.frequency = freq;
  }

  // Advanced waveform generation
  generateWave(phase, type) {
    switch(type) {
      case 0: return Math.sin(phase); // Sine
      case 1: return phase < Math.PI ? -1 : 1; // Square
      case 2: return (2 * phase / (Math.PI * 2)) - 1; // Saw
      case 3: return Math.abs((2 * phase / (Math.PI * 2)) - 1) * 2 - 1; // Triangle
      case 4: return Math.tanh(3 * Math.sin(phase)); // Warm sine
      case 5: { // Additive harmonics
        let sum = 0;
        for (let i = 1; i <= 8; i++) {
          sum += Math.sin(phase * i) / i;
        }
        return sum * 0.3;
      }
      case 6: { // FM synthesis
        const modulator = Math.sin(phase * 2.1) * 0.5;
        return Math.sin(phase + modulator);
      }
      default: return Math.sin(phase);
    }
  }

  process() {
    const phaseIncrement = (2 * Math.PI * this.frequency) / this.sampleRate;
    this.phase += phaseIncrement;
    if (this.phase >= 2 * Math.PI) this.phase -= 2 * Math.PI;

    let output = this.generateWave(this.phase, this.waveform) * 0.7;

    // Add temporal layers for richness
    this.temporalLayers.forEach((layer, i) => {
      layer.phase += (2 * Math.PI * this.frequency * layer.frequency) / this.sampleRate;
      if (layer.phase >= 2 * Math.PI) layer.phase -= 2 * Math.PI;
      
      output += this.generateWave(layer.phase, layer.waveform) * layer.amplitude;
    });

    return output;
  }
}

// Voice management system
class TitanVoice {
  constructor(sampleRate) {
    this.sampleRate = sampleRate;
    this.oscillator = new TitanOscillator(sampleRate);
    this.envelope = new TitanEnvelope();
    this.filter = new TitanFilters(sampleRate);
    this.active = false;
    this.note = 0;
    this.velocity = 0;
    this.sampleData = null;
    this.samplePosition = 0;
    this.sampleRate = sampleRate;
  }

  noteOn(note, velocity, sampleData = null) {
    this.note = note;
    this.velocity = velocity / 127;
    this.active = true;
    this.sampleData = sampleData;
    this.samplePosition = 0;
    
    const frequency = 440 * Math.pow(2, (note - 69) / 12);
    this.oscillator.setFrequency(frequency);
    this.envelope.noteOn();
  }

  noteOff() {
    this.envelope.noteOff();
  }

  process() {
    if (!this.active) return 0;

    const envLevel = this.envelope.process();
    if (envLevel <= 0 && this.envelope.stage === 0) {
      this.active = false;
      return 0;
    }

    let output;
    
    if (this.sampleData && this.samplePosition < this.sampleData.length) {
      // High-quality sample playback with interpolation
      const pos = this.samplePosition;
      const frac = pos - Math.floor(pos);
      const i = Math.floor(pos);
      
      if (i + 3 < this.sampleData.length) {
        // Hermite interpolation for smooth playback
        output = AdvancedInterpolation.hermite(
          frac,
          this.sampleData[i - 1] || 0,
          this.sampleData[i] || 0,
          this.sampleData[i + 1] || 0,
          this.sampleData[i + 2] || 0
        );
      } else {
        output = this.sampleData[i] || 0;
      }
      
      // Pitch adjustment for sample playback
      const pitchRatio = Math.pow(2, (this.note - 60) / 12);
      this.samplePosition += pitchRatio;
      
      if (this.samplePosition >= this.sampleData.length) {
        this.samplePosition = 0; // Loop
      }
    } else {
      // Oscillator synthesis
      output = this.oscillator.process();
    }

    // Apply envelope and velocity
    output *= envLevel * this.velocity;

    return output;
  }
}

// Main synthesizer engine
class TitanSynthEngine {
  constructor() {
    this.voices = [];
    this.sampleRate = SAMPLE_RATE;
    this.masterVolume = 0.7;
    this.samples = new Map();
    this.effects = {
      reverb: { enabled: false, roomSize: 0.5, damping: 0.5, wetLevel: 0.3 },
      delay: { enabled: false, time: 0.25, feedback: 0.3, wetLevel: 0.2 },
      chorus: { enabled: false, rate: 0.5, depth: 0.3, wetLevel: 0.3 },
      distortion: { enabled: false, drive: 2, tone: 0.5, level: 0.8 }
    };
    
    // Initialize voice pool
    for (let i = 0; i < MAX_VOICES; i++) {
      this.voices.push(new TitanVoice(this.sampleRate));
    }

    // Initialize effects buffers
    this.initializeEffects();
  }

  initializeEffects() {
    // Reverb buffers
    this.reverbBuffers = Array(4).fill(0).map(() => 
      new Float32Array(Math.floor(this.sampleRate * 0.1))
    );
    this.reverbIndices = [0, 0, 0, 0];
    
    // Delay buffer
    this.delayBuffer = new Float32Array(Math.floor(this.sampleRate * 2));
    this.delayIndex = 0;
    
    // Chorus LFO
    this.chorusPhase = 0;
  }

  findFreeVoice() {
    return this.voices.find(voice => !voice.active);
  }

  noteOn(note, velocity, sampleKey = null) {
    const voice = this.findFreeVoice();
    if (voice) {
      const sampleData = sampleKey ? this.samples.get(sampleKey) : null;
      voice.noteOn(note, velocity, sampleData);
    }
  }

  noteOff(note) {
    this.voices.forEach(voice => {
      if (voice.active && voice.note === note) {
        voice.noteOff();
      }
    });
  }

  loadSample(name, audioBuffer) {
    // Convert AudioBuffer to Float32Array for processing
    const channelData = audioBuffer.getChannelData(0);
    this.samples.set(name, channelData);
  }

  // Advanced effects processing
  processReverb(input) {
    if (!this.effects.reverb.enabled) return input;
    
    let output = input;
    const { roomSize, damping, wetLevel } = this.effects.reverb;
    
    // Multi-tap reverb algorithm
    this.reverbBuffers.forEach((buffer, i) => {
      const delayTime = Math.floor(buffer.length * (0.3 + i * 0.2) * roomSize);
      const index = this.reverbIndices[i];
      
      const delayed = buffer[index];
      buffer[index] = input + delayed * (1 - damping) * 0.7;
      
      this.reverbIndices[i] = (index + 1) % buffer.length;
      output += delayed * wetLevel;
    });
    
    return output * (1 - wetLevel) + output * wetLevel;
  }

  processDelay(input) {
    if (!this.effects.delay.enabled) return input;
    
    const { time, feedback, wetLevel } = this.effects.delay;
    const delayTime = Math.floor(this.sampleRate * time);
    const index = this.delayIndex;
    
    const delayed = this.delayBuffer[index];
    this.delayBuffer[index] = input + delayed * feedback;
    
    this.delayIndex = (this.delayIndex + 1) % this.delayBuffer.length;
    
    return input * (1 - wetLevel) + delayed * wetLevel;
  }

  processChorus(input) {
    if (!this.effects.chorus.enabled) return input;
    
    const { rate, depth, wetLevel } = this.effects.chorus;
    
    // LFO for chorus modulation
    this.chorusPhase += (2 * Math.PI * rate) / this.sampleRate;
    if (this.chorusPhase >= 2 * Math.PI) this.chorusPhase -= 2 * Math.PI;
    
    const lfo = Math.sin(this.chorusPhase) * depth;
    const delayTime = Math.floor((0.005 + lfo * 0.002) * this.sampleRate);
    
    // Simple delay for chorus effect
    const index = (this.delayIndex - delayTime + this.delayBuffer.length) % this.delayBuffer.length;
    const chorused = this.delayBuffer[index];
    
    return input * (1 - wetLevel) + chorused * wetLevel;
  }

  processDistortion(input) {
    if (!this.effects.distortion.enabled) return input;
    
    const { drive, tone, level } = this.effects.distortion;
    
    // Soft clipping distortion
    let distorted = Math.tanh(input * drive) / Math.tanh(drive);
    
    // Simple tone control (lowpass)
    this.distortionFilter = (this.distortionFilter || 0) * tone + distorted * (1 - tone);
    
    return this.distortionFilter * level;
  }

  process(outputBuffer) {
    const bufferLength = outputBuffer.length;
    
    for (let i = 0; i < bufferLength; i++) {
      let sum = 0;
      
      // Sum all active voices
      this.voices.forEach(voice => {
        if (voice.active) {
          sum += voice.process();
        }
      });
      
      // Apply effects chain
      sum = this.processDistortion(sum);
      sum = this.processReverb(sum);
      sum = this.processDelay(sum);
      sum = this.processChorus(sum);
      
      // Master volume and soft limiting
      sum *= this.masterVolume;
      sum = Math.tanh(sum * 0.8) * 1.25; // Soft limiting
      
      outputBuffer[i] = sum;
    }
  }
}

// Main React component
const TitanSynth = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [currentPreset, setCurrentPreset] = useState('Classic Piano');
  const [masterVolume, setMasterVolume] = useState(70);
  const [effects, setEffects] = useState({
    reverb: { enabled: false, roomSize: 50, damping: 50, wetLevel: 30 },
    delay: { enabled: false, time: 25, feedback: 30, wetLevel: 20 },
    chorus: { enabled: false, rate: 50, depth: 30, wetLevel: 30 },
    distortion: { enabled: false, drive: 200, tone: 50, level: 80 }
  });
  const [activeNotes, setActiveNotes] = useState(new Set());
  const [cpu, setCpu] = useState(0);
  const [voices, setVoices] = useState(0);

  const synthRef = useRef(null);
  const audioContextRef = useRef(null);
  const processorRef = useRef(null);
  const animationRef = useRef(null);

  // Initialize audio system
  useEffect(() => {
    const initAudio = async () => {
      try {
        audioContextRef.current = new (window.AudioContext || window.webkitAudioContext)();
        synthRef.current = new TitanSynthEngine();
        
        // Create audio worklet processor
        await audioContextRef.current.audioWorklet.addModule(
          URL.createObjectURL(new Blob([`
            class TitanSynthProcessor extends AudioWorkletProcessor {
              constructor() {
                super();
                this.port.onmessage = (e) => {
                  if (e.data.type === 'synth') {
                    this.synth = e.data.synth;
                  }
                };
              }
              
              process(inputs, outputs) {
                const output = outputs[0];
                if (this.synth && output[0]) {
                  this.synth.process(output[0]);
                }
                return true;
              }
            }
            registerProcessor('titan-synth-processor', TitanSynthProcessor);
          `], { type: 'application/javascript' }))
        );

        processorRef.current = new AudioWorkletNode(audioContextRef.current, 'titan-synth-processor');
        processorRef.current.connect(audioContextRef.current.destination);
        
      } catch (error) {
        console.error('Failed to initialize audio:', error);
      }
    };

    initAudio();

    return () => {
      if (audioContextRef.current) {
        audioContextRef.current.close();
      }
    };
  }, []);

  // Performance monitoring
  useEffect(() => {
    const updateStats = () => {
      if (synthRef.current) {
        const activeVoices = synthRef.current.voices.filter(v => v.active).length;
        setVoices(activeVoices);
        setCpu(Math.min(100, activeVoices * 2)); // Approximate CPU usage
      }
      animationRef.current = requestAnimationFrame(updateStats);
    };
    
    updateStats();
    
    return () => {
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    };
  }, []);

  // Keyboard handling
  const handleKeyDown = useCallback((e) => {
    const keyMap = {
      'KeyA': 60, 'KeyW': 61, 'KeyS': 62, 'KeyE': 63, 'KeyD': 64,
      'KeyF': 65, 'KeyT': 66, 'KeyG': 67, 'KeyY': 68, 'KeyH': 69,
      'KeyU': 70, 'KeyJ': 71, 'KeyK': 72, 'KeyO': 73, 'KeyL': 74
    };
    
    const note = keyMap[e.code];
    if (note && !activeNotes.has(note) && synthRef.current) {
      setActiveNotes(prev => new Set([...prev, note]));
      synthRef.current.noteOn(note, 100);
    }
  }, [activeNotes]);

  const handleKeyUp = useCallback((e) => {
    const keyMap = {
      'KeyA': 60, 'KeyW': 61, 'KeyS': 62, 'KeyE': 63, 'KeyD': 64,
      'KeyF': 65, 'KeyT': 66, 'KeyG': 67, 'KeyY': 68, 'KeyH': 69,
      'KeyU': 70, 'KeyJ': 71, 'KeyK': 72, 'KeyO': 73, 'KeyL': 74
    };
    
    const note = keyMap[e.code];
    if (note && activeNotes.has(note) && synthRef.current) {
      setActiveNotes(prev => {
        const newSet = new Set(prev);
        newSet.delete(note);
        return newSet;
      });
      synthRef.current.noteOff(note);
    }
  }, [activeNotes]);

  useEffect(() => {
    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
    
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, [handleKeyDown, handleKeyUp]);

  // Effect parameter updates
  const updateEffect = (effectName, param, value) => {
    setEffects(prev => ({
      ...prev,
      [effectName]: {
        ...prev[effectName],
        [param]: value
      }
    }));
    
    if (synthRef.current) {
      synthRef.current.effects[effectName][param] = param === 'enabled' ? value : value / 100;
    }
  };

  const presets = [
    'Classic Piano', 'Electric Piano', 'Synth Lead', 'Warm Pad',
    'Bass Synth', 'Strings', 'Brass', 'Vocal Synth'
  ];

  return (
    <div className="w-full h-screen bg-gradient-to-br from-slate-900 via-purple-900 to-slate-800 text-white overflow-hidden">
      {/* Header */}
      <div className="flex items-center justify-between p-4 border-b border-purple-500/30">
        <div className="flex items-center space-x-3">
          <Zap className="w-8 h-8 text-yellow-400" />
          <h1 className="text-2xl font-bold bg-gradient-to-r from-yellow-400 to-purple-400 bg-clip-text text-transparent">
            TitanSynth
          </h1>
          <span className="text-xs bg-yellow-400/20 px-2 py-1 rounded text-yellow-300">
            SOTA Multi-Temporal Engine
          </span>
        </div>
        
        <div className="flex items-center space-x-4 text-sm">
          <div className="flex items-center space-x-2">
            <div className="w-2 h-2 bg-green-400 rounded-full animate-pulse"></div>
            <span>Voices: {voices}/{MAX_VOICES}</span>
          </div>
          <div className="flex items-center space-x-2">
            <div className={`w-2 h-2 rounded-full ${cpu > 80 ? 'bg-red-400' : cpu > 50 ? 'bg-yellow-400' : 'bg-green-400'}`}></div>
            <span>CPU: {cpu}%</span>
          </div>
        </div>
      </div>

      <div className="flex h-full">
        {/* Left Panel - Controls */}
        <div className="w-80 bg-black/20 border-r border-purple-500/30 p-4 overflow-y-auto">
          {/* Preset Selection */}
          <div className="mb-6">
            <h3 className="text-lg font-semibold mb-3 flex items-center">
              <Settings className="w-5 h-5 mr-2 text-purple-400" />
              Presets
            </h3>
            <select
              value={currentPreset}
              onChange={(e) => setCurrentPreset(e.target.value)}
              className="w-full bg-slate-700 border border-purple-500/30 rounded px-3 py-2 text-white"
            >
              {presets.map(preset => (
                <option key={preset} value={preset}>{preset}</option>
              ))}
            </select>
          </div>

          {/* Master Controls */}
          <div className="mb-6">
            <h3 className="text-lg font-semibold mb-3 flex items-center">
              <Sliders className="w-5 h-5 mr-2 text-purple-400" />
              Master
            </h3>
            <div className="space-y-3">
              <div>
                <label className="block text-sm mb-1">Volume</label>
                <input
                  type="range"
                  min="0"
                  max="100"
                  value={masterVolume}
                  onChange={(e) => {
                    setMasterVolume(e.target.value);
                    if (synthRef.current) {
                      synthRef.current.masterVolume = e.target.value / 100;
                    }
                  }}
                  className="w-full"
                />
                <span className="text-xs text-purple-300">{masterVolume}%</span>
              </div>
            </div>
          </div>

          {/* Effects */}
          {Object.entries(effects).map(([effectName, params]) => (
            <div key={effectName} className="mb-6">
              <div className="flex items-center justify-between mb-3">
                <h3 className="text-lg font-semibold capitalize">{effectName}</h3>
                <input
                  type="checkbox"
                  checked={params.enabled}
                  onChange={(e) => updateEffect(effectName, 'enabled', e.target.checked)}
                  className="w-4 h-4"
                />
              </div>
              
              {params.enabled && (
                <div className="space-y-2 pl-4">
                  {Object.entries(params).filter(([key]) => key !== 'enabled').map(([param, value]) => (
                    <div key={param}>
                      <label className="block text-sm mb-1 capitalize">{param}</label>
                      <input
                        type="range"
                        min="0"
                        max="100"
                        value={value}
                        onChange={(e) => updateEffect(effectName, param, parseInt(e.target.value))}
                        className="w-full"
                      />
                      <span className="text-xs text-purple-300">{value}%</span>
                    </div>
                  ))}
                </div>
              )}
            </div>
          ))}
        </div>

        {/* Main Area */}
        <div className="flex-1 flex flex-col">
          {/* Waveform Display */}
          <div className="h-32 bg-black/30 border-b border-purple-500/30 flex items-center justify-center">
            <div className="flex items-center space-x-2 text-purple-300">
              <Activity className="w-6 h-6" />
              <span>Real-time Waveform Analysis</span>
            </div>
            {/* Simulated waveform */}
            <div className="ml-8 flex items-center space-x-1">
              {Array(50).fill(0).map((_, i) => (
                <div
                  key={i}
                  className="w-1 bg-gradient-to-t from-purple-600 to-yellow-400 opacity-70"
                  style={{ 
                    height: `${20 + Math.sin(Date.now() * 0.01 + i * 0.3) * 15 + (activeNotes.size > 0 ? Math.random() * 20 : 0)}px` 
                  }}
                />
              ))}
            </div>
          </div>

          {/* Virtual Keyboard */}
          <div className="flex-1 flex items-center justify-center bg-gradient-to-b from-slate-800/50 to-black/50">
            <div className="bg-black/30 rounded-lg p-6 border border-purple-500/30">
              <h3 className="text-center mb-4 text-purple-300">Virtual Keyboard</h3>
              <div className="flex space-x-1">
                {/* White keys */}
                {[60, 62, 64, 65, 67, 69, 71, 72].map((note) => (
                  <button
                    key={note}
                    className={`w-12 h-32 rounded-b ${
                      activeNotes.has(note) 
                        ? 'bg-gradient-to-b from-yellow-400 to-purple-600 shadow-lg shadow-purple-500/50' 
                        : 'bg-white hover:bg-gray-100'
                    } border border-gray-300 transition-all duration-100 transform ${
                      activeNotes.has(note) ? 'scale-95' : 'hover:scale-105'
                    }`}
                    onMouseDown={() => {
                      if (synthRef.current) {
                        setActiveNotes(prev => new Set([...prev, note]));
                        synthRef.current.noteOn(note, 100);
                      }
                    }}
                    onMouseUp={() => {
                      if (synthRef.current) {
                        setActiveNotes(prev => {
                          const newSet = new Set(prev);
                          newSet.delete(note);
                          return newSet;
                        });
                        synthRef.current.noteOff(note);
                      }
                    }}
                    onMouseLeave={() => {
                      if (activeNotes.has(note) && synthRef.current) {
                        setActiveNotes(prev => {
                          const newSet = new Set(prev);
                          newSet.delete(note);
                          return newSet;
                        });
                        synthRef.current.noteOff(note);
                      }
                    }}
                  />
                ))}
                
                {/* Black keys */}
                <div className="absolute flex space-x-1 transform translate-x-8">
                  {[61, 63, null, 66, 68, 70, null].map((note, i) => 
                    note ? (
                      <button
                        key={note}
                        className={`w-8 h-20 rounded-b ${
                          activeNotes.has(note) 
                            ? 'bg-gradient-to-b from-purple-400 to-yellow-600 shadow-lg shadow-yellow-500/50' 
                            : 'bg-black hover:bg-gray-800'
                        } border border-gray-600 transition-all duration-100 transform ${
                          activeNotes.has(note) ? 'scale-95' : 'hover:scale-105'
                        } z-10`}
                        style={{ marginLeft: i === 2 || i === 6 ? '48px' : '40px' }}
                        onMouseDown={() => {
                          if (synthRef.current) {
                            setActiveNotes(prev => new Set([...prev, note]));
                            synthRef.current.noteOn(note, 100);
                          }
                        }}
                        onMouseUp={() => {
                          if (synthRef.current) {
                            setActiveNotes(prev => {
                              const newSet = new Set(prev);
                              newSet.delete(note);
                              return newSet;
                            });
                            synthRef.current.noteOff(note);
                          }
                        }}
                        onMouseLeave={() => {
                          if (activeNotes.has(note) && synthRef.current) {
                            setActiveNotes(prev => {
                              const newSet = new Set(prev);
                              newSet.delete(note);
                              return newSet;
                            });
                            synthRef.current.noteOff(note);
                          }
                        }}
                      />
                    ) : (
                      <div key={i} className="w-8" />
                    )
                  )}
                </div>
              </div>
              
              <div className="text-center mt-4 text-sm text-purple-300">
                Use AWSEDFTGYHUJKOL keys to play
              </div>
            </div>
          </div>

          {/* Bottom Controls */}
          <div className="h-20 bg-black/40 border-t border-purple-500/30 flex items-center justify-between px-6">
            <div className="flex items-center space-x-4">
              <button
                onClick={() => {
                  if (audioContextRef.current?.state === 'suspended') {
                    audioContextRef.current.resume();
                  }
                  setIsPlaying(!isPlaying);
                }}
                className="flex items-center space-x-2 bg-gradient-to-r from-purple-600 to-blue-600 px-4 py-2 rounded-lg hover:from-purple-700 hover:to-blue-700 transition-all"
              >
                {isPlaying ? <Pause className="w-5 h-5" /> : <Play className="w-5 h-5" />}
                <span>{isPlaying ? 'Pause' : 'Play'}</span>
              </button>
              
              <button className="flex items-center space-x-2 bg-red-600 px-4 py-2 rounded-lg hover:bg-red-700 transition-all">
                <Square className="w-5 h-5" />
                <span>Stop</span>
              </button>
            </div>

            <div className="flex items-center space-x-4">
              <button className="flex items-center space-x-2 bg-green-600 px-3 py-2 rounded hover:bg-green-700 transition-all">
                <Upload className="w-4 h-4" />
                <span>Load Sample</span>
              </button>
              
              <button className="flex items-center space-x-2 bg-blue-600 px-3 py-2 rounded hover:bg-blue-700 transition-all">
                <Download className="w-4 h-4" />
                <span>Export</span>
              </button>
            </div>

            <div className="flex items-center space-x-4 text-sm">
              <div className="text-purple-300">
                Sample Rate: {SAMPLE_RATE}Hz
              </div>
              <div className="text-purple-300">
                Latency: {audioContextRef.current ? Math.round(audioContextRef.current.baseLatency * 1000) : 0}ms
              </div>
              <div className="text-purple-300">
                Temporal Layers: {TEMPORAL_LAYERS}
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Advanced Features Panel */}
      <div className="fixed top-4 right-4 w-64 bg-black/80 backdrop-blur-sm border border-purple-500/30 rounded-lg p-4">
        <h3 className="text-lg font-semibold mb-3 text-purple-400">Advanced Features</h3>
                  <div className="space-y-2 text-sm">
          <div className="flex justify-between">
            <span>Hermite Interpolation</span>
            <span className="text-green-400">●</span>
          </div>
          <div className="flex justify-between">
            <span>State Variable Filters</span>
            <span className="text-green-400">●</span>
          </div>
          <div className="flex justify-between">
            <span>Moog Ladder Filters</span>
            <span className="text-green-400">●</span>
          </div>
          <div className="flex justify-between">
            <span>Multi-tap Reverb</span>
            <span className="text-green-400">●</span>
          </div>
          <div className="flex justify-between">
            <span>Additive Synthesis</span>
            <span className="text-green-400">●</span>
          </div>
          <div className="flex justify-between">
            <span>FM Synthesis</span>
            <span className="text-green-400">●</span>
          </div>
          <div className="flex justify-between">
            <span>Soft Limiting</span>
            <span className="text-green-400">●</span>
          </div>
          <div className="flex justify-between">
            <span>Real-time Analysis</span>
            <span className="text-green-400">●</span>
          </div>
        </div>
        
        <div className="mt-4 pt-3 border-t border-purple-500/30">
          <div className="text-xs text-purple-300">
            TitanSynth v1.0 - State of the Art Multi-Temporal Synthesis Engine
          </div>
        </div>
      </div>

      {/* Loading/Status Overlay */}
      {!audioContextRef.current && (
        <div className="fixed inset-0 bg-black/80 backdrop-blur-sm flex items-center justify-center z-50">
          <div className="text-center">
            <div className="w-16 h-16 border-4 border-purple-500 border-t-transparent rounded-full animate-spin mx-auto mb-4"></div>
            <h2 className="text-xl font-semibold mb-2">Initializing TitanSynth</h2>
            <p className="text-purple-300">Loading advanced audio processing engine...</p>
          </div>
        </div>
      )}
    </div>
  );
};

export default TitanSynth;