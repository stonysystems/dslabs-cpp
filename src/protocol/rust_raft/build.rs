// build.rs - Build script for generating Rust bindings from C++ headers
// This runs before compilation and generates bindings.rs using bindgen

// NOTE: bindgen is currently commented out in Cargo.toml
// Uncomment the bindgen dependency in Cargo.toml to enable this

fn main() {
    // Tell cargo to rerun this build script if any of these files change
    println!("cargo:rerun-if-changed=../raft/server.h");
    println!("cargo:rerun-if-changed=../raft/commo.h");
    println!("cargo:rerun-if-changed=../constants.h");
    println!("cargo:rerun-if-changed=../raft/frame.h");

    // Uncomment below to enable bindgen
    /*
    use std::env;
    use std::path::PathBuf;

    // Generate bindings from C++ headers
    let bindings = bindgen::Builder::default()
        // Input header files
        .header("../raft/server.h")
        .header("../raft/commo.h")
        .header("../constants.h")

        // Add include paths for C++ headers
        .clang_arg("-I../")
        .clang_arg("-I../../")
        .clang_arg("-I../../../")

        // Enable C++ mode
        .clang_arg("-std=c++17")
        .clang_arg("-xc++")

        // Allowlist specific types/functions we want to bind
        .allowlist_type("ballot_t")
        .allowlist_type("slotid_t")
        .allowlist_type("siteid_t")
        .allowlist_type("parid_t")
        .allowlist_type("bool_t")
        .allowlist_type("RaftData")

        // Generate bindings
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    // Write bindings to $OUT_DIR/bindings.rs
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
    */
}
