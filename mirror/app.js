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
  dying: "受击",
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
    loadImage("failure", "../assets/devil-jump-failure.png"),
    loadImage("score", "../assets/devil-jump-score-bg.png"),
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
  if (data.mode === "CHALLENGE") return "CHALLENGE";
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
  for (const p of data.platforms || []) {
    const [x, y, w, type = 0, crumbling = 0, crumbleTicks = 0] = p;
    const color = platformColor(data.level);

    if (crumbling) {
      const fall = crumbleTicks * 2;
      const half = Math.max(5, Math.floor(w / 2) - 3);
      ctx.fillStyle = "#061018";
      roundRect(x - 1, y + fall - 1, half + 1, 8, 2, true);
      ctx.fillStyle = "#c47c4d";
      roundRect(x, y + fall, half, 6, 2, true);
      ctx.fillStyle = "#061018";
      roundRect(x + half + 5, y + fall + 2, w - half - 5, 8, 2, true);
      ctx.fillStyle = "#78624d";
      roundRect(x + half + 6, y + fall + 3, w - half - 7, 6, 2, true);
      ctx.fillStyle = "#ff9d35";
      ctx.fillRect(x + half + 2, y + fall + 6, 2, 2);
      ctx.fillRect(x + w - 5, y + fall + 10, 2, 2);
      continue;
    }

    ctx.fillStyle = "#061018";
    roundRect(x - 1, y - 2, w + 2, 8, 3, true);

    if (type === 0) {
      ctx.fillStyle = "#ffffff";
      for (let i = 0; i < w; i += 10) {
        ctx.beginPath();
        ctx.arc(x + i + 5, y + 1, 5, 0, Math.PI * 2);
        ctx.fill();
      }
      ctx.fillStyle = "#8edcff";
      roundRect(x, y + 1, w, 5, 3, true);
    } else if (type === 1) {
      ctx.fillStyle = "#9b6a24";
      roundRect(x, y, w, 7, 2, true);
      ctx.fillStyle = "#8cff57";
      roundRect(x, y - 1, w, 4, 2, true);
      for (let i = 3; i < w; i += 8) ctx.fillRect(x + i, y - 4, 1, 4);
    } else if (type === 2) {
      ctx.fillStyle = "#e63b25";
      roundRect(x, y + 1, w, 6, 3, true);
      ctx.fillStyle = "#ff9d35";
      roundRect(x + 2, y - 2, Math.max(1, w - 4), 5, 5, true);
      ctx.fillStyle = "#ffe761";
      ctx.fillRect(x + 5, y - 1, Math.max(1, w - 10), 1);
    } else if (type === 3) {
      ctx.fillStyle = "#42a3ff";
      roundRect(x, y, w, 6, 2, true);
      ctx.fillStyle = "#c5ffff";
      for (let i = 0; i < w; i += 7) {
        ctx.beginPath();
        ctx.moveTo(x + i, y + 6);
        ctx.lineTo(x + i + 4, y - 2);
        ctx.lineTo(x + i + 8, y + 6);
        ctx.fill();
      }
    } else if (type === 5) {
      ctx.fillStyle = "#78624d";
      roundRect(x, y, w, 7, 2, true);
      ctx.fillStyle = "#d79a61";
      roundRect(x, y - 1, w, 5, 2, true);
      ctx.strokeStyle = "#ff4141";
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(x + w / 2 - 7, y - 1);
      ctx.lineTo(x + w / 2 - 3, y + 2);
      ctx.lineTo(x + w / 2 + 1, y);
      ctx.lineTo(x + w / 2 + 6, y + 3);
      ctx.stroke();
    } else {
      ctx.fillStyle = "#3b1235";
      roundRect(x, y, w, 6, 2, true);
      ctx.fillStyle = "#ffe761";
      roundRect(x, y - 1, w, 4, 2, true);
      ctx.fillStyle = "#ffffff";
      for (let i = 4; i < w; i += 15) ctx.fillRect(x + i, y, 1, 1);
    }
  }
}

function drawMiniStar(x, y, color) {
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.moveTo(x, y - 5);
  ctx.lineTo(x + 2, y - 1);
  ctx.lineTo(x + 5, y);
  ctx.lineTo(x + 2, y + 1);
  ctx.lineTo(x, y + 5);
  ctx.lineTo(x - 2, y + 1);
  ctx.lineTo(x - 5, y);
  ctx.lineTo(x - 2, y - 1);
  ctx.fill();
}

function drawResultIcon(x, y, type, color) {
  ctx.fillStyle = color;
  if (type === 0) drawRocket(x - 4, y - 9, false);
  else if (type === 1) drawMonster(x - 8, y - 8, 1);
  else if (type === 2) {
    roundRect(x - 8, y - 4, 8, 7, 1, true);
    ctx.fillStyle = "#78624d";
    roundRect(x + 2, y - 5, 8, 8, 1, true);
  } else if (type === 3) {
    ctx.fillRect(x - 5, y - 7, 2, 14);
    ctx.beginPath();
    ctx.moveTo(x - 3, y - 7);
    ctx.lineTo(x + 8, y - 4);
    ctx.lineTo(x - 3, y);
    ctx.fill();
  } else {
    ctx.beginPath();
    ctx.moveTo(x, y - 9);
    ctx.lineTo(x - 8, y + 7);
    ctx.lineTo(x + 8, y + 7);
    ctx.fill();
  }
}

function drawStatTile(x, y, label, value, type, color) {
  ctx.fillStyle = "#10222d";
  roundRect(x, y, 54, 33, 4, true);
  ctx.strokeStyle = "#3a394c";
  roundRect(x, y, 54, 33, 4, false);
  drawResultIcon(x + 13, y + 16, type, color);
  ctx.font = "bold 7px system-ui, sans-serif";
  ctx.textAlign = "left";
  ctx.fillStyle = "#c8f5ff";
  ctx.fillText(label, x + 25, y + 12);
  ctx.textAlign = "right";
  ctx.fillStyle = "#ffffff";
  ctx.fillText(String(value ?? 0), x + 52, y + 27);
}

function drawResultPanel(data, title) {
  const previousBest = data.prevBest ?? data.best ?? 0;
  const score = data.score ?? 0;
  const newBest = score > previousBest;
  const delta = score - previousBest;

  drawCover(images.score);
  ctx.textAlign = "center";
  ctx.font = "bold 30px system-ui, sans-serif";
  ctx.fillStyle = "#000000";
  ctx.fillText(String(score), logical.w / 2 + 2, 130);
  ctx.fillStyle = "#ffe761";
  ctx.fillText(String(score), logical.w / 2, 128);

  ctx.font = "bold 8px system-ui, sans-serif";
  ctx.fillStyle = newBest ? "#ffe761" : "#c8f5ff";
  ctx.fillText(newBest ? `NEW +${delta}` : `BEST ${data.best ?? previousBest}`, logical.w / 2, 186);
}

function drawMonster(x, y, type = 0) {
  const body = ["#7b1bb8", "#13d17a", "#f04a3a"][type] || "#7b1bb8";
  const belly = ["#d44aff", "#77f0a4", "#ff9d35"][type] || "#d44aff";
  ctx.fillStyle = "#061018";
  roundRect(x, y + 1, 16, 13, 5, true);
  ctx.fillStyle = body;
  roundRect(x + 1, y, 14, 12, 5, true);
  ctx.fillStyle = "#ffe761";
  ctx.beginPath();
  ctx.moveTo(x + 1, y + 1);
  ctx.lineTo(x + 4, y - 5);
  ctx.lineTo(x + 7, y + 1);
  ctx.fill();
  ctx.beginPath();
  ctx.moveTo(x + 15, y + 1);
  ctx.lineTo(x + 12, y - 5);
  ctx.lineTo(x + 9, y + 1);
  ctx.fill();
  ctx.fillStyle = belly;
  ctx.beginPath();
  ctx.arc(x + 8, y + 8, 4, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = "#fff";
  ctx.fillRect(x + 5, y + 4, 2, 2);
  ctx.fillRect(x + 10, y + 4, 2, 2);
  ctx.fillStyle = "#061018";
  ctx.fillRect(x + 6, y + 10, 5, 1);
}

function drawRocket(x, y, flameOn = false) {
  ctx.fillStyle = "#ffe761";
  ctx.beginPath();
  ctx.moveTo(x + 4, y - 3);
  ctx.lineTo(x + 1, y + 3);
  ctx.lineTo(x + 7, y + 3);
  ctx.fill();
  ctx.fillStyle = "#f04a3a";
  roundRect(x + 1, y + 2, 7, 14, 3, true);
  ctx.fillStyle = "#ff9d35";
  roundRect(x + 3, y + 4, 3, 8, 1, true);
  ctx.fillStyle = "#9fb8c8";
  ctx.fillRect(x, y + 10, 2, 5);
  ctx.fillRect(x + 7, y + 10, 2, 5);
  if (flameOn) {
    ctx.fillStyle = "#39d6ff";
    ctx.beginPath();
    ctx.moveTo(x + 2, y + 15);
    ctx.lineTo(x + 7, y + 15);
    ctx.lineTo(x + 4, y + 24);
    ctx.fill();
    ctx.fillStyle = "#ffe761";
    ctx.beginPath();
    ctx.moveTo(x + 3, y + 15);
    ctx.lineTo(x + 6, y + 15);
    ctx.lineTo(x + 4, y + 21);
    ctx.fill();
  }
}

function drawHazards(data) {
  for (const monster of data.monsters || []) {
    drawMonster(monster[0], monster[1], monster[2] || 0);
  }

  for (const rocket of data.rockets || []) {
    drawRocket(rocket[0], rocket[1], false);
  }

  const bullets = data.bullets || (data.bullet ? [data.bullet] : []);
  for (const bullet of bullets) {
    const [x, y] = bullet;
    ctx.fillStyle = "#ffe761";
    roundRect(x, y, 3, 7, 1, true);
    ctx.fillStyle = "#fff";
    ctx.fillRect(x + 1, y - 1, 1, 1);
    ctx.fillStyle = "#ff9d35";
    ctx.fillRect(x + 1, y + 7, 1, 1);
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
  else ctx.stroke();
}

function drawPlayer(data) {
  const [x, y] = data.player || [0, 0];
  if (data.rocket) {
    drawRocket(x + 11, y + 16, true);
  }
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
  } else if (data.state === "gameover") {
    ctx.fillStyle = "#10222d";
    ctx.fillRect(0, 0, logical.w, logical.h);
    drawResultPanel(data, "SCORE");
  } else if (data.state === "dying") {
    drawCover(images.failure || images[`level${data.level || 1}`]);
    const phase = confettiTick % 48;
    ctx.strokeStyle = phase < 24 ? "#ff4141" : "#ffe761";
    ctx.lineWidth = 2;
    ctx.fillStyle = "#1c2429";
    roundRect(18, logical.h - 48, logical.w - 36, 30, 6, true);
    ctx.strokeStyle = phase < 24 ? "#ff4141" : "#ffe761";
    roundRect(18, logical.h - 48, logical.w - 36, 30, 6, false);
    ctx.font = "bold 7px system-ui, sans-serif";
    ctx.textAlign = "center";
    ctx.fillStyle = "#ffffff";
    ctx.fillText(phase < 37 ? "OUCH" : "SCORE NEXT", logical.w / 2, logical.h - 36);
    ctx.fillStyle = "#05081d";
    roundRect(22, logical.h - 27, logical.w - 44, 3, 1, true);
    ctx.fillStyle = "#ffe761";
    roundRect(22, logical.h - 27, Math.min(logical.w - 44, ((phase + 1) / 48) * (logical.w - 44)), 3, 1, true);
  } else if (data.state === "victory") {
    drawCover(images.victory);
    drawConfetti();
    drawResultPanel(data, "SUCCESS");
  } else if (data.state === "ascend") {
    drawCover(images[`level${data.level || 1}`]);
    drawPlatforms(data);
    drawHazards(data);
    drawAngelPlayer(data);
    drawHud(data);
    drawBottomText("ASCENDING");
  } else if (data.state === "waiting" || data.state === "connected" || data.state === "error") {
    drawCover(images.home);
    drawBottomText(data.message || "等待 M5Stick");
  } else {
    drawCover(images[`level${data.level || 1}`]);
    drawPlatforms(data);
    drawHazards(data);
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
  if (latest?.state === "victory" || latest?.state === "dying") {
    render(latest);
  }
  requestAnimationFrame(animate);
}

loadAssets();
connectBridge();
animate();
