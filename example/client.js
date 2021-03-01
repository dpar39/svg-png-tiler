function createElmt(elmtType) {
  const SVG_NS = "http://www.w3.org/2000/svg";
  return document.createElementNS(SVG_NS, elmtType);
}
function initSvg(width, height) {
  const svg = createElmt("svg");
  const h = height.toString();
  const w = width.toString();
  svg.setAttribute("width", w);
  svg.setAttribute("height", h);
  return svg;
}

function addLine(svg, x1, y1, x2, y2, stroke) {
  let line = createElmt("line");
  line.setAttribute("x1", x1.toString());
  line.setAttribute("y1", y1.toString());
  line.setAttribute("x2", x2.toString());
  line.setAttribute("y2", y2.toString());
  line.setAttribute("stroke", stroke);
  svg.appendChild(line);
  return line;
}

function addText(svg, x, y, value, fill) {
  let text = createElmt("text");
  text.setAttribute("x", x.toString());
  text.setAttribute("y", y.toString());
  text.setAttribute("fill", fill || "lightgray");
  text.appendChild(document.createTextNode(value));
  svg.appendChild(text);
  return text;
}

function addRect(svg, x, y, w, h, fill) {
  let rect = createElmt("rect");
  rect.setAttribute("x", x.toString());
  rect.setAttribute("y", y.toString());
  rect.setAttribute("width", w.toString());
  rect.setAttribute("height", h.toString());
  rect.setAttribute("fill", fill);

  svg.appendChild(rect);
  return rect;
}

function addImage(svg, src, x, y, width, height) {
  const img = createElmt("image");
  img.setAttribute("width", width.toString());
  img.setAttribute("height", height.toString());
  img.setAttribute("x", x.toString());
  img.setAttribute("y", y.toString());
  img.setAttributeNS("http://www.w3.org/1999/xlink", "xlink:href", src);
  svg.appendChild(img);
  return img;
}

function svgElementToString(svgElement) {
  return new XMLSerializer().serializeToString(svgElement);
}

function makeSvg(width, height, backgroundColor) {
  const svg = initSvg(width, height);

  if (backgroundColor) {
    const r = addRect(svg, 1, 1, width - 3, height - 3, backgroundColor);
    r.setAttribute("stroke", "yellow");
    r.setAttribute("stroke-width", "3px");
    r.setAttribute("fill", "#222222");
  }

  const gridColor = "darkgray";
  const offset = 10;
  const grid = 200;
  for (let x = offset; ; x += grid) {
    addLine(svg, x, offset, x, height - offset, gridColor);
    if (x > width - offset) {
      x = width - offset;
      addLine(svg, x, offset, x, height - offset, gridColor);
      break;
    }
  }
  for (let y = offset; ; y += grid) {
    addLine(svg, offset, y, width - offset, y, gridColor);
    if (y > height - offset) {
      y = height - offset;
      addLine(svg, offset, y, width - offset, y, gridColor);
      break;
    }
  }

  for (let x = offset; x < width; x += grid)
    for (let y = offset; y < height; y += grid) {
      if (y < grid || x > width - 60) {
        continue;
      }
      const t = addText(svg, x + 4, y - 5, `${x - offset}, ${y - offset}`);
      t.setAttribute("stroke", "orangered");
    }
  return svg;
}

function saveContent(filename, fileContent) {
  const a = document.createElement("a");
  a.style.display = "none";
  document.body.appendChild(a);
  const blob = new Blob([fileContent], {
    type: "text/xml+svg;charset=utf-8",
  });
  const objectURL = URL.createObjectURL(blob);
  a.href = objectURL;
  a.href = URL.createObjectURL(blob);
  a.download = filename;
  a.click();
  document.body.removeChild(a);
}

function downloadURL(data, fileName) {
  var a;
  a = document.createElement("a");
  a.href = data;
  a.download = fileName;
  document.body.appendChild(a);
  a.style = "display: none";
  a.click();
  a.remove();
}

function downloadBlob(data, fileName, mimeType) {
  var blob, url;
  blob = new Blob([data], {
    type: mimeType,
  });
  url = window.URL.createObjectURL(blob);
  downloadURL(url, fileName);
  return window.URL.revokeObjectURL(url);
}

function blobToDataUrl(blob, mime) {
  return new Promise((resolve) => {
    const reader = new FileReader();
    reader.onloadend = () => {
      const GENERIC_MIME = "data:application/octet-stream";
      if (mime && reader.result.startsWith(GENERIC_MIME)) {
        const len = GENERIC_MIME.length;
        resolve(`data:${mime}` + reader.result.substr(len));
      } else {
        resolve(reader.result);
      }
    };
    reader.readAsDataURL(blob);
  });
}

async function urlToDataUrl(url) {
  let res = await fetch(url);
  const contentType = res.headers.get("Content-Type");
  let blob = await res.blob();
  return await blobToDataUrl(blob, contentType);
}

async function resolveXLinksHRef(svg) {
  let reg = /xlink:href="(https?:([A-z0-9~:,/?#\[\]@\!\$&'\(\)\*\+\s;%\.=]+))"/g;
  let out = svg;
  const resolvedUrls = new Map();
  while ((result = reg.exec(svg)) !== null) {
    let url = result[1];
    let dataUrl = null;
    if (resolvedUrls.has(url)) {
      dataUrl = resolvedUrls.get(url);
    } else {
      dataUrl = await urlToDataUrl(url);
      resolvedUrls.set(url, dataUrl);
    }
    out = out.replace(url, dataUrl);
  }
  return out;
}

function createBlobUrl(svgElmt) {
  const xmlSerializer = new XMLSerializer();
  const svgString = xmlSerializer.serializeToString(svgElmt);
  const svgBlob = new Blob([svgString], {
    type: "image/svg+xml;charset=utf-8",
  });
  return URL.createObjectURL(svgBlob);
}

function loadSvgOnImage(img, svgElmt) {
  return new Promise((resolve, reject) => {
    img.onload = () => resolve();
    img.onerror = () => reject();
    img.src = createBlobUrl(svgElmt);
  });
}

async function createSvgWithPngTiles(svgElmt, xa, ya, xSize, ySize) {
  // Make tiles so that rows don't use more than the area of an 8K image
  const Dim16K = Math.pow(2, 14);
  const Area8kImage = 7680 * 4320;
  const tileSizeX = Math.min(Dim16K, xSize); // 16K max
  const tileSizeY = Math.min(Dim16K, Math.floor(Area8kImage / xSize), ySize); // 16K max

  const finalSvg = initSvg(xSize, ySize);

  for (let j = 0; j < ySize / tileSizeY; ++j) {
    let y = ya + j * tileSizeY;
    let tileY = Math.min(tileSizeY, ySize - j * tileSizeY);
    let tileYStr = tileY.toString();

    for (let i = 0; i < xSize / tileSizeX; ++i) {
      let x = xa + i * tileSizeX;
      let tileX = Math.min(tileSizeX, xSize - i * tileSizeX);
      let tileXStr = tileX.toString();

      svgElmt.setAttribute("width", tileXStr);
      svgElmt.setAttribute("height", tileYStr);
      svgElmt.setAttribute("viewBox", `${x} ${y} ${tileXStr} ${tileYStr}`);

      let img = new Image();
      img.width = tileXStr + "px";
      img.height = tileYStr + "px";

      await loadSvgOnImage(img, svgElmt);

      let canvas = document.createElement("canvas");
      canvas.width = tileX;
      canvas.height = tileY;
      let ctx = canvas.getContext("2d");
      ctx.drawImage(img, 0, 0);
      addImage(
        finalSvg,
        canvas.toDataURL(),
        i * tileSizeX,
        j * tileSizeY,
        tileX,
        tileY
      );
      URL.revokeObjectURL(img.src);
    }
  }
  saveContent("finalSvg.svg", new XMLSerializer().serializeToString(finalSvg));
}

async function exportSvgPngTiles(svg) {
  svg = await resolveXLinksHRef(svg);
  const svgDocument = new DOMParser().parseFromString(svg, "image/svg+xml");
  const svgElmt = svgDocument.firstChild;
  let style = svgDocument.createElementNS("http://www.w3.org/2000/svg", "style");
  //style.textContent = await getEmbeddedFonts(svg);
  // svgElmt.insertBefore(style, svgElmt);

  const vw = parseInt(svgElmt.getAttribute("width"));
  const vh = parseInt(svgElmt.getAttribute("height"));
  await createSvgWithPngTiles(svgElmt, 0, 0, vw, vh);
}
