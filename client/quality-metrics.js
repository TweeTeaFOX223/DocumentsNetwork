const qualityVisualColorMap = {
    SE: "#d45555",
    MDS: "#2b7a78",
    CE: "#3b82a6",
    KK: "#8b5fbf"
};

function normalizeQualityVisualKey(visual)
{
    const text = String(visual || "").toUpperCase();
    if (text.includes("(SE)") || text.includes("SE")) return "SE";
    if (text.includes("(MDS)") || text.includes("MDS")) return "MDS";
    if (text.includes("(CE)") || text.includes("CE")) return "CE";
    if (text.includes("(KK)") || text.includes("KK")) return "KK";
    return String(visual || "?");
}

function escapeQualityHtml(text)
{
    return String(text || "")
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;");
}

function getVisualizationQualityMetrics(response)
{
    if (!response || !response.body || typeof response.body !== "object") return null;
    const body = response.body;
    if (!Array.isArray(body.edges) || !Array.isArray(body.nodes)) return null;

    const edgeLengths = body.edges.map((edge) => {
        const dx = Number(edge.x2) - Number(edge.x1);
        const dy = Number(edge.y2) - Number(edge.y1);
        return Math.sqrt(dx * dx + dy * dy);
    }).filter((value) => Number.isFinite(value));

    const edgeCount = edgeLengths.length;
    const meanEdgeLength = edgeCount > 0
        ? edgeLengths.reduce((sum, value) => sum + value, 0) / edgeCount
        : 0;
    const variance = edgeCount > 1
        ? edgeLengths.reduce((sum, value) => sum + Math.pow(value - meanEdgeLength, 2), 0) / edgeCount
        : 0;
    const stdDev = Math.sqrt(Math.max(0, variance));
    const cv = meanEdgeLength > 0 ? (stdDev / meanEdgeLength) * 100 : 0;

    let overlapPairs = 0;
    for (let i = 0; i < body.nodes.length; i++) {
        for (let j = i + 1; j < body.nodes.length; j++) {
            const left = body.nodes[i];
            const right = body.nodes[j];
            const dx = Number(left.cx) - Number(right.cx);
            const dy = Number(left.cy) - Number(right.cy);
            const distance = Math.sqrt(dx * dx + dy * dy);
            const radiusSum = Number(left.r || 0) + Number(right.r || 0);
            if (distance <= radiusSum) overlapPairs += 1;
        }
    }

    return {
        edgeCount,
        nodeCount: body.nodes.length,
        meanEdgeLength,
        stdDevEdgeLength: stdDev,
        cv,
        overlapPairs
    };
}

function getQualitySeriesFromLogs(requestLogs)
{
    return requestLogs
        .map((log, index) => {
            const metrics = getVisualizationQualityMetrics(log.response);
            if (!metrics) return null;
            const generation = log.generation || {};
            const visual = generation.visual || log.request?.networkType || "?";
            const visualKey = normalizeQualityVisualKey(visual);
            const label = [
                visual,
                generation.graph || log.request?.graphType || "?",
                generation.search || (log.request?.searchNum ? `${log.request.searchNum}件` : "?")
            ].join(" / ");
            return {
                index,
                cv: metrics.cv,
                overlapPairs: metrics.overlapPairs,
                label,
                visual,
                visualKey,
                isLatest: index === requestLogs.length - 1
            };
        })
        .filter(Boolean);
}

function renderQualityScatter(points)
{
    if (!points.length) {
        return `<div class="quality-empty">評価対象のグラフがまだありません。</div>`;
    }

    const width = 900;
    const height = 340;
    const margin = { top: 18, right: 18, bottom: 52, left: 86 };
    const plotWidth = width - margin.left - margin.right;
    const plotHeight = height - margin.top - margin.bottom;
    const maxCv = Math.max(1, ...points.map((point) => point.cv));
    const maxOverlap = Math.max(1, ...points.map((point) => point.overlapPairs));
    const xMax = maxCv * 1.1;
    const yMax = Math.max(1, maxOverlap * 1.1);
    const xTicks = 4;
    const yTicks = 4;

    const xPos = (value) => margin.left + (value / xMax) * plotWidth;
    const yPos = (value) => margin.top + plotHeight - (value / yMax) * plotHeight;

    const xGrid = Array.from({ length: xTicks + 1 }, (_, i) => {
        const value = (xMax / xTicks) * i;
        const x = xPos(value);
        return `
            <line x1="${x}" y1="${margin.top}" x2="${x}" y2="${margin.top + plotHeight}" stroke="#e3e8ee" stroke-width="1"></line>
            <text x="${x}" y="${height - 18}" text-anchor="middle" font-size="11" fill="#667788">${value.toFixed(1)}</text>
        `;
    }).join("");
    const yGrid = Array.from({ length: yTicks + 1 }, (_, i) => {
        const value = (yMax / yTicks) * i;
        const y = yPos(value);
        return `
            <line x1="${margin.left}" y1="${y}" x2="${margin.left + plotWidth}" y2="${y}" stroke="#e3e8ee" stroke-width="1"></line>
            <text x="${margin.left - 16}" y="${y + 4}" text-anchor="end" font-size="11" fill="#667788">${value.toFixed(1)}</text>
        `;
    }).join("");
    const legendItems = Array.from(new Set(points.map((point) => point.visual)))
        .map((visual) => {
            const color = qualityVisualColorMap[normalizeQualityVisualKey(visual)] || "#667788";
            return `
                <div style="display:flex; align-items:center; gap:6px;">
                    <span style="display:inline-block; width:12px; height:12px; border-radius:50%; background:${color};"></span>
                    <span>${escapeQualityHtml(visual)}</span>
                </div>
            `;
        }).join("");

    const dots = points.map((point) => {
        const x = xPos(point.cv);
        const y = yPos(point.overlapPairs);
        const baseFill = qualityVisualColorMap[point.visualKey] || "#667788";
        const fill = baseFill;
        const radius = point.isLatest ? 6 : 4.5;
        const stroke = point.isLatest ? "#111111" : "#ffffff";
        const strokeWidth = point.isLatest ? 2 : 1.2;
        return `
            <g>
                <title>${escapeQualityHtml(`${point.label} | cv=${point.cv.toFixed(2)} | on=${point.overlapPairs}`)}</title>
                <circle cx="${x}" cy="${y}" r="${radius}" fill="${fill}" fill-opacity="0.92" stroke="${stroke}" stroke-width="${strokeWidth}"></circle>
            </g>
        `;
    }).join("");

    return `
        <div class="quality-scatter-wrap">
            <div class="quality-scatter-title">蓄積されたグラフの比較散布図（左下ほど高評価）</div>
            <div style="display:flex; flex-wrap:wrap; align-items:center; gap:12px; margin:0 0 8px 0; font-size:12px; color:#445566;">
                <span style="font-weight:700;">凡例</span>
                ${legendItems}
                <div style="display:flex; align-items:center; gap:6px;">
                    <span style="display:inline-block; width:12px; height:12px; border-radius:50%; background:#ffffff; border:2px solid #111111;"></span>
                    <span>最新生成</span>
                </div>
            </div>
            <svg class="quality-scatter-svg" viewBox="0 0 ${width} ${height}" role="img" aria-label="visualization quality scatter plot">
                ${xGrid}
                ${yGrid}
                <rect x="${margin.left}" y="${margin.top}" width="${plotWidth}" height="${plotHeight}" fill="transparent" stroke="#9cadbd" stroke-width="1.5"></rect>
                ${dots}
                <text x="${margin.left + plotWidth / 2}" y="${height - 4}" text-anchor="middle" font-size="12" fill="#425466">評価項目1: エッジ長の変動係数 cv</text>
                <text x="24" y="${margin.top + plotHeight / 2}" text-anchor="middle" font-size="12" fill="#425466" transform="rotate(-90 24 ${margin.top + plotHeight / 2})">評価項目2: 重なりペア数 on</text>
            </svg>
        </div>
    `;
}
