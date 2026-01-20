# MIDI Mappings

## Clock (handled by Base)

Kastle 2 automatically syncs to the MIDI clock if it is present on the USB (or TRS – Citadel) it takes the highest priority over the internal and external sync clock. When using multiple MIDI sources, the first which sends the MIDI clock will be used. In the advanced settings there is an option to ignore the MIDI clock and clock multipliers. That's handled all by Base feature `MIDI_CLOCK`.

Kastle 2 outputs MIDI clock over USB, when in internal clock mode or external using a jack or patchbay. When there is an external MIDI clock source, Kastle 2 doesn't send any clock over USB MIDI.

## Note Input (handled by Apps)

By default most apps use MIDI notes 0-47 to switch between modes or banks. Note 48 and above usually triggers a sound. See the apps implementation.

## CC Input (handled by Base & Apps)

Settings Main Volume (CC7), Input Gain (CC9), LFO (CC22), LFO Mod (CC23), Tempo (CC24) and Rhythm (CC25) are handled by Base. All other are handled by Apps.

## Note Output (handled by Apps)

App dependant.

## CC Output (handled by Base & Apps)

The all seven knobs (in normal Layer, non-shifted) are by default mapped to these values and sent by Base:  
14, 15, 16, 17, 18, 22, 23


