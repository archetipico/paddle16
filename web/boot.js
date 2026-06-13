"use strict";

const screen = document.getElementById("screen");
const overlay = document.getElementById("overlay");
const power = document.getElementById("power");
const reset = document.getElementById("reset");
const full = document.getElementById("full");

const emulator = new V86({
  wasm_path: "vendor/v86/v86.wasm",
  bios: { url: "vendor/seabios/seabios.bin" },
  vga_bios: { url: "vendor/vgabios/vgabios.bin" },
  fda: { url: "paddle16.img" },
  screen_container: screen,
  autostart: false,
  disable_mouse: true,
});

emulator.add_listener("emulator-loaded", () => {
  power.disabled = false;
  power.querySelector(".power__label").textContent = "power on";
});

emulator.add_listener("screen-set-size", (s) => {
  if (s && s[0] >= 320 && s[1] >= 200) {
    setTimeout(() => { if (overlay.dataset.state === "booting") overlay.dataset.state = "off"; }, 150);
  }
});

let booted = false;
let shuttingDown = false;

function start() {
  shuttingDown = false;
  screen.classList.remove("off");
  document.body.classList.add("running");
  overlay.dataset.state = "booting";
  if (booted) emulator.restart();
  else { emulator.run(); booted = true; }
  reset.disabled = false;
  full.disabled = false;
  screen.focus();
}

function shutdown() {
  if (shuttingDown || overlay.dataset.state !== "off") return;
  shuttingDown = true;
  screen.classList.add("off");
  screen.addEventListener("animationend", () => {
    document.body.classList.remove("running");
    overlay.dataset.state = "idle";
  }, { once: true });
}

setInterval(() => {
  try { if (overlay.dataset.state === "off" && emulator.v86.cpu.in_hlt[0] === 1) shutdown(); } catch (e) {}
}, 250);

power.addEventListener("click", () => { if (!power.disabled) start(); });
reset.addEventListener("click", () => { overlay.dataset.state = "booting"; emulator.restart(); screen.focus(); });
full.addEventListener("click", () => {
  if (document.fullscreenElement) document.exitFullscreen();
  else document.documentElement.requestFullscreen();
});

const PAD = {
  w:     { d: [0x11], u: [0x91] },
  s:     { d: [0x1f], u: [0x9f] },
  up:    { d: [0xe0, 0x48], u: [0xe0, 0xc8] },
  down:  { d: [0xe0, 0x50], u: [0xe0, 0xd0] },
  space: { d: [0x39], u: [0xb9] },
  p:     { d: [0x19], u: [0x99] },
};

document.querySelectorAll(".pad__btn").forEach((btn) => {
  const k = PAD[btn.dataset.k];
  const down = (e) => { e.preventDefault(); emulator.keyboard_send_scancodes(k.d); };
  const up = (e) => { e.preventDefault(); emulator.keyboard_send_scancodes(k.u); };
  btn.addEventListener("pointerdown", down);
  btn.addEventListener("pointerup", up);
  btn.addEventListener("pointercancel", up);
  btn.addEventListener("pointerleave", up);
});

screen.tabIndex = 0;
