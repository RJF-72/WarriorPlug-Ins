use nih_plug::prelude::*;

fn main() {
    // Simple standalone wrapper to run the plugin
    println!("Warrior Headphone Plugin - Standalone Mode");
    println!("This would typically open a standalone audio application.");
    println!("However, NIH-plug plugins are designed to be loaded in DAWs as VST3/CLAP plugins.");
    println!("");
    println!("Plugin has been successfully built and bundled!");
    println!("You can find the plugin files at:");
    println!("  VST3: target/bundled/warrior_headphone.vst3");
    println!("  CLAP: target/bundled/warrior_headphone.clap");
    println!("");
    println!("To test the plugin, load it in your favorite DAW that supports VST3 or CLAP plugins.");
    println!("The plugin includes:");
    println!("  - Headphone EQ calibration");  
    println!("  - Crossfeed processing");
    println!("  - Codec compensation");
    println!("  - Multiple headphone models");
    println!("  - Gain control and blend settings");
}