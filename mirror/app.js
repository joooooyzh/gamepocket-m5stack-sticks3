const canvas = document.querySelector("#stage");
const ctx = canvas.getContext("2d");
const connectButton = document.querySelector("#connect");
const stateEl = document.querySelector("#state");
const levelEl = document.querySelector("#level");
const scoreEl = document.querySelector("#score");
const bestEl = document.querySelector("#best");
const hintEl = document.querySelector("#hint");

ctx.imageSmoothingEnabled = false;

const logical = { w: 135, h: 240 };
const images = {};
const stateNames = {
  home: "首页",
  play: "游戏中",
  ascend: "飞升",
  gameover: "Game Over",
  victory: "Success",
  waiting: "等待串口",
  connected: "桥接已连接",
  error: "串口错误",
};

let latest = null;
let confettiTick = 0;
let bridgeConnected = false;
const bridgeUrl = "http://127.0.0.1:8766/mirror/";

function loadImage(name, src) {
  return new Promise((resolve) => {
    const img = new Image();
    img.onload = () => {
      images[name] = img;
      resolve();
    };
    img.onerror = () => resolve();
    img.src = src;
  });
}

async function loadAssets() {
  const tasks = [
    loadImage("home", "../assets/devil-jump-home.png"),
    loadImage("gameover", "../assets/devil-jump-gameover.png"),
    loadImage("victory", "../assets/devil-jump-victory.png"),
    loadImage("player", "../assets/player-devil-clean.png?v=3"),
  ];

  for (let i = 1; i <= 7; i += 1) {
    const names = ["sky", "cloud", "sunset", "dusk", "high", "stars", "space"];
    tasks.push(loadImage(`level${i}`, `../assets/level-${i}-${names[i - 1]}.png`));
  }

  await Promise.all(tasks);
  drawIdle();
}

function drawCover(img) {
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, logical.w, logical.h);
  if (!img) return;

  const scale = Math.max(logical.w / img.width, logical.h / img.height);
  const w = img.width * scale;
  const h = img.height * scale;
  const x = (logical.w - w) / 2;
  const y = (logical.h - h) / 2;
  ctx.drawImage(img, x, y, w, h);
}

function drawIdle() {
  drawCover(images.home);
  drawBottomText("等待连接");
}

function drawBottomText(text) {
  let size = 8;
  do {
    ctx.font = `bold ${size}px system-ui, sans-serif`;
    size -= 1;
  } while (ctx.measureText(text).width > logical.w - 8 && size >= 5);
  ctx.textAlign = "center";
  ctx.lineWidth = 3;
  ctx.strokeStyle = "#081020";
  ctx.fillStyle = "#ffffff";
  ctx.strokeText(text, logical.w / 2, logical.h - 10);
  ctx.fillText(text, logical.w / 2, logical.h - 10);
}

function hudColor(level) {
  return level >= 4 ? "#ffffff" : "#172033";
}

function platformColor(level) {
  return ["#a5ff54", "#4df8ff", "#ffdd38", "#ff9d35", "#ffffff", "#ff5dff", "#ffe761"][level - 1] || "#ffffff";
}

function difficultyLabel(data) {
  if (data.mode === "EASY") return "EASY";
  if (data.mode === "STANDARD") return "STANDARD";
  if (data.mode === "HARD") return `HARD ${"★".repeat(data.stars || 1)}`;
  return "";
}

function drawHud(data) {
  const color = hudColor(data.level);
  ctx.font = "bold 8px system-ui, sans-serif";
  ctx.textAlign = "left";
  ctx.lineWidth = 2;
  ctx.strokeStyle = color === "#ffffff" ? "#081020" : "rgba(255,255,255,0.85)";
  ctx.fillStyle = color;

  const left = `L${data.level} ${data.score}`;
  const alt = data.alt || "";
  ctx.strokeText(left, 4, 11);
  ctx.fillText(left, 4, 11);
  ctx.strokeText(alt, 4, 21);
  ctx.fillText(alt, 4, 21);
  const mode = difficultyLabel(data);
  ctx.strokeText(mode, 4, 31);
  ctx.fillText(mode, 4, 31);

  const best = `B ${data.best}`;
  const width = ctx.measureText(best).width;
  ctx.strokeText(best, logical.w - width - 4, 11);
  ctx.fillText(best, logical.w - width - 4, 11);
}

function drawPlatforms(data) {
  const color = platformColor(data.level);
  for (const p of data.platforms || []) {
    const [x, y, w] = p;
    ctx.fillStyle = "#061018";
    roundRect(x - 1, y - 1, w + 2, 6, 2, true);
    ctx.fillStyle = color;
    roundRect(x, y, w, 4, 2, true);
    ctx.fillStyle = "#ffffff";
    ctx.fillRect(x, y, w, 1);
  }
}

function roundRect(x, y, w, h, r, fill) {
  ctx.beginPath();
  ctx.moveTo(x + r, y);
  ctx.arcTo(x + w, y, x + w, y + h, r);
  ctx.arcTo(x + w, y + h, x, y + h, r);
  ctx.arcTo(x, y + h, x, y, r);
  ctx.arcTo(x, y, x + w, y, r);
  if (fill) ctx.fill();
}

function drawPlayer(data) {
  const [x, y] = data.player || [0, 0];
  if (images.player) {
    ctx.drawImage(images.player, x, y, 30, 34);
    return;
  }
  ctx.fillStyle = "#f04a3a";
  ctx.fillRect(x, y, 14, 16);
}

function drawAngelPlayer(data) {
  const [x, y] = data.player || [0, 0];
  const cx = x + 15;
  ctx.fillStyle = "#ffffff";
  ctx.strokeStyle = "#ffe45c";
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(x - 8, y + 15);
  ctx.lineTo(x + 4, y + 7);
  ctx.lineTo(x + 6, y + 27);
  ctx.closePath();
  ctx.fill();
  ctx.stroke();
  ctx.beginPath();
  ctx.moveTo(x + 38, y + 15);
  ctx.lineTo(x + 26, y + 7);
  ctx.lineTo(x + 24, y + 27);
  ctx.closePath();
  ctx.fill();
  ctx.stroke();
  ctx.beginPath();
  ctx.ellipse(cx, y - 3, 10, 3, 0, 0, Math.PI * 2);
  ctx.strokeStyle = "#ffe45c";
  ctx.stroke();
  drawPlayer(data);
}

function drawConfetti() {
  const colors = ["#ff4141", "#ffe24d", "#4dff88", "#39d6ff", "#ff66d8", "#ffffff"];
  for (let i = 0; i < 34; i += 1) {
    const x = (i * 29 + confettiTick * 0.55) % logical.w;
    const y = (confettiTick * 1.8 + i * 17) % (logical.h + 26) - 20;
    ctx.fillStyle = colors[(i + Math.floor(confettiTick / 12)) % colors.length];
    if (i % 3 === 0) ctx.fillRect(x, y, 3, 7);
    else if (i % 3 === 1) ctx.fillRect(x, y, 7, 3);
    else {
      ctx.beginPath();
      ctx.moveTo(x, y);
      ctx.lineTo(x + 5, y + 2);
      ctx.lineTo(x + 1, y + 6);
      ctx.fill();
    }
  }
}

function render(data) {
  if (!data) {
    drawIdle();
    return;
  }

  if (data.state === "home") {
    drawCover(images.home);
    drawBottomText(`${difficultyLabel(data) || "HARD ★★★★★"}  A MODE  B START`);
  } else if (data.state === "gameover") {
    drawCover(images.gameover);
    drawBottomText("A/B HOME");
  } else if (data.state === "victory") {
    drawCover(images.victory);
    drawConfetti();
    drawBottomText("SUCCESS");
  } else if (data.state === "ascend") {
    drawCover(images[`level${data.level || 1}`]);
    drawPlatforms(data);
    drawAngelPlayer(data);
    drawHud(data);
    drawBottomText("ASCENDING");
  } else if (data.state === "waiting" || data.state === "connected" || data.state === "error") {
    drawCover(images.home);
    drawBottomText(data.message || "等待 M5Stick");
  } else {
    drawCover(images[`level${data.level || 1}`]);
    drawPlatforms(data);
    drawPlayer(data);
    drawHud(data);
  }

  stateEl.textContent = stateNames[data.state] || data.state || "未知";
  levelEl.textContent = data.level ? `L${data.level} ${difficultyLabel(data)}` : difficultyLabel(data) || "-";
  scoreEl.textContent = data.score ?? 0;
  bestEl.textContent = data.best ?? 0;
}

async function connectSerial() {
  if (location.protocol === "file:") {
    location.href = bridgeUrl;
    return;
  }

  if (bridgeConnected) {
    hintEl.textContent = "桥接服务已经连接，不需要手动选择串口。";
    return;
  }

  if (!("serial" in navigator)) {
    hintEl.textContent = "当前浏览器不支持手动串口连接；请使用本地桥接服务。";
    return;
  }

  const port = await navigator.serial.requestPort();
  await port.open({ baudRate: 115200 });

  connectButton.textContent = "已连接";
  connectButton.disabled = true;
  hintEl.textContent = "正在同步 M5Stick 画面。请不要同时打开 PlatformIO 串口监视。";

  const decoder = new TextDecoderStream();
  port.readable.pipeTo(decoder.writable).catch(() => {});
  const reader = decoder.readable.getReader();
  let buffer = "";

  while (true) {
    const { value, done } = await reader.read();
    if (done) break;
    buffer += value;
    const lines = buffer.split(/\r?\n/);
    buffer = lines.pop() || "";
    for (const line of lines) {
      if (!line.startsWith("DJ:")) continue;
      try {
        latest = JSON.parse(line.slice(3));
        render(latest);
      } catch {
        // Ignore partial serial lines.
      }
    }
  }
}

function connectBridge() {
  if (location.protocol === "file:") {
    hintEl.textContent = "当前是直接打开文件，不能自动同步。请点击按钮打开桥接页面。";
    connectButton.textContent = "打开桥接页面";
    stateEl.textContent = "请切换地址";
    return;
  }

  const events = new EventSource("/events");

  events.onopen = () => {
    bridgeConnected = true;
    connectButton.textContent = "桥接已连接";
    hintEl.textContent = "正在同步 M5Stick 画面。请不要同时打开 PlatformIO 串口监视。";
  };

  events.onmessage = (event) => {
    try {
      latest = JSON.parse(event.data);
      render(latest);
      if (latest.message && (latest.state === "error" || latest.state === "waiting")) {
        hintEl.textContent = latest.message;
      }
    } catch {
      // Ignore malformed bridge packets.
    }
  };

  events.onerror = () => {
    if (!bridgeConnected) {
      hintEl.textContent = "没有检测到桥接服务。可以启动 tools/serial_mirror.py，或用 Chrome/Edge 手动连接串口。";
    }
  };
}

connectButton.addEventListener("click", () => {
  if (location.protocol === "file:") {
    location.href = bridgeUrl;
    return;
  }

  connectSerial().catch((error) => {
    hintEl.textContent = `连接失败：${error.message}`;
    connectButton.disabled = false;
    connectButton.textContent = "重新连接";
  });
});

function animate() {
  confettiTick += 1;
  if (latest?.state === "victory") {
    render(latest);
  }
  requestAnimationFrame(animate);
}

loadAssets();
connectBridge();
animate();
