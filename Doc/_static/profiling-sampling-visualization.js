/**
 * Sampling Profiler Visualization
 */
(function () {
  "use strict";

  // ============================================================================
  // Configuration
  // ============================================================================

  const TIMINGS = {
    sampleIntervalMin: 100,
    sampleIntervalMax: 500,
    sampleIntervalDefault: 200,
    sampleToFlame: 600,
    defaultSpeed: 0.05,
  };

  const LAYOUT = { frameSpacing: 6 };

  // Function name to color mapping
  const FUNCTION_COLORS = {
    main: "#306998",
    fibonacci: "#D4A910",
    add: "#E65100",
    multiply: "#7B1FA2",
    calculate: "#D4A910",
  };
  const DEFAULT_FUNCTION_COLOR = "#306998";

  // Easing functions - cubic-bezier approximations
  const EASING_MAP = {
    linear: "linear",
    easeOutQuad: "cubic-bezier(0.25, 0.46, 0.45, 0.94)",
    easeOutCubic: "cubic-bezier(0.215, 0.61, 0.355, 1)",
  };

  function getFunctionColor(funcName) {
    return FUNCTION_COLORS[funcName] || DEFAULT_FUNCTION_COLOR;
  }

  // ============================================================================
  // Animation Manager
  // ============================================================================

  class AnimationManager {
    constructor() {
      this.activeAnimations = new Set();
    }

    to(element, props, duration, easing = "easeOutQuad", onComplete = null) {
      this.killAnimationsOf(element);

      const cssEasing = EASING_MAP[easing] || EASING_MAP.easeOutQuad;

      const transformProps = {};
      const otherProps = {};

      for (const [key, value] of Object.entries(props)) {
        if (key === "position") {
          if (typeof value.x === "number") transformProps.x = value.x;
          if (typeof value.y === "number") transformProps.y = value.y;
        } else if (key === "x" || key === "y") {
          transformProps[key] = value;
        } else if (key === "scale") {
          transformProps.scale = value;
        } else if (key === "alpha" || key === "opacity") {
          otherProps.opacity = value;
        } else {
          otherProps[key] = value;
        }
      }

      const computedStyle = getComputedStyle(element);
      const matrix = new DOMMatrix(computedStyle.transform);
      const currentScale = Math.sqrt(
        matrix.m11 * matrix.m11 + matrix.m21 * matrix.m21,
      );

      transformProps.x ??= matrix.m41;
      transformProps.y ??= matrix.m42;
      transformProps.scale ??= currentScale;

      const initialTransform = this._buildTransformString(
        matrix.m41,
        matrix.m42,
        currentScale,
      );

      const finalTransform = this._buildTransformString(
        transformProps.x,
        transformProps.y,
        transformProps.scale,
      );

      const initialKeyframe = { transform: initialTransform };
      const finalKeyframe = { transform: finalTransform };

      for (const [key, value] of Object.entries(otherProps)) {
        const currentVal =
          key === "opacity"
            ? element.style.opacity || computedStyle.opacity
            : element.style[key];
        initialKeyframe[key] = currentVal;
        finalKeyframe[key] = value;
      }

      const animation = element.animate([initialKeyframe, finalKeyframe], {
        duration,
        easing: cssEasing,
        fill: "forwards",
      });

      this.activeAnimations.add(animation);
      animation.onfinish = () => {
        this.activeAnimations.delete(animation);
        element.style.transform = finalTransform;
        for (const [key, value] of Object.entries(finalKeyframe)) {
          if (key !== "transform") {
            element.style[key] = typeof value === "number" ? `${value}` : value;
          }
        }
        if (onComplete) onComplete();
      };

      return animation;
    }

    killAnimationsOf(element) {
      element.getAnimations().forEach((animation) => animation.cancel());
      this.activeAnimations.forEach((animation) => {
        if (animation.effect && animation.effect.target === element) {
          animation.cancel();
          this.activeAnimations.delete(animation);
        }
      });
    }

    _buildTransformString(x, y, scale = 1) {
      return `translate(${x}px, ${y}px) scale(${scale})`;
    }
  }

  const anim = new AnimationManager();

  // ============================================================================
  // Execution Trace Model
  // ============================================================================

  class ExecutionEvent {
    constructor(
      type,
      functionName,
      lineno,
      timestamp,
      args = null,
      value = null,
    ) {
      this.type = type;
      this.functionName = functionName;
      this.lineno = lineno;
      this.timestamp = timestamp;
      this.args = args;
      this.value = value;
    }
  }

  class ExecutionTrace {
    constructor(source, events) {
      this.source = source;
      this.events = events.map(
        (e) =>
          new ExecutionEvent(e.type, e.func, e.line, e.ts, e.args, e.value),
      );
      this.duration = events.length > 0 ? events[events.length - 1].ts : 0;
    }

    getEventsUntil(timestamp) {
      return this.events.filter((e) => e.timestamp <= timestamp);
    }

    getStackAt(timestamp) {
      const stack = [];
      const events = this.getEventsUntil(timestamp);

      for (const event of events) {
        if (event.type === "call") {
          stack.push({
            func: event.functionName,
            line: event.lineno,
            args: event.args,
          });
        } else if (event.type === "return") {
          stack.pop();
        } else if (event.type === "line") {
          if (stack.length > 0) {
            stack[stack.length - 1].line = event.lineno;
          }
        }
      }
      return stack;
    }

    getNextEvent(timestamp) {
      return this.events.find((e) => e.timestamp > timestamp);
    }

    getSourceLines() {
      return this.source.split("\n");
    }
  }

  // ============================================================================
  // Demo Data
  // ============================================================================

  // This placeholder is replaced by the profiling_trace Sphinx extension
  // during the documentation build with dynamically generated trace data.
  const DEMO_SIMPLE = /* PROFILING_TRACE_DATA */ null;

  // ============================================================================
  // Code Panel Component
  // ============================================================================

  class CodePanel {
    constructor(source) {
      this.source = source;
      this.currentLine = null;

      this.element = document.createElement("div");
      this.element.id = "code-panel";

      const title = document.createElement("div");
      title.className = "code-panel-title";
      title.textContent = "source code";
      this.element.appendChild(title);

      this.codeContainer = document.createElement("pre");
      this.codeContainer.className = "code-container";
      this.element.appendChild(this.codeContainer);

      this._renderSource();
    }

    updateSource(source) {
      this.source = source;
      this.codeContainer.innerHTML = "";
      this._renderSource();
      this.currentLine = null;
    }

    _renderSource() {
      const lines = this.source.split("\n");

      lines.forEach((line, index) => {
        const lineNumber = index + 1;
        const lineDiv = document.createElement("div");
        lineDiv.className = "line";
        lineDiv.dataset.line = lineNumber;

        const lineNumSpan = document.createElement("span");
        lineNumSpan.className = "line-number";
        lineNumSpan.textContent = lineNumber;
        lineDiv.appendChild(lineNumSpan);

        const codeSpan = document.createElement("span");
        codeSpan.className = "line-content";
        codeSpan.innerHTML = this._highlightSyntax(line);
        lineDiv.appendChild(codeSpan);

        this.codeContainer.appendChild(lineDiv);
      });
    }

    _highlightSyntax(line) {
      return line
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/(f?"[^"]*"|f?'[^']*')/g, '<span class="string">$1</span>')
        .replace(/(#.*$)/g, '<span class="comment">$1</span>')
        .replace(
          /\b(def|if|elif|else|return|for|in|range|print|__name__|__main__)\b/g,
          '<span class="keyword">$1</span>',
        )
        .replace(
          /<span class="keyword">def<\/span>\s+(\w+)/g,
          '<span class="keyword">def</span> <span class="function">$1</span>',
        )
        .replace(/\b(\d+)\b/g, '<span class="number">$1</span>');
    }

    highlightLine(lineNumber) {
      if (this.currentLine === lineNumber) return;

      if (this.currentLine !== null) {
        const prevLine = this.codeContainer.querySelector(
          `[data-line="${this.currentLine}"]`,
        );
        if (prevLine) prevLine.classList.remove("highlighted");
      }

      if (lineNumber === null || lineNumber === undefined) {
        this.currentLine = null;
        return;
      }

      this.currentLine = lineNumber;
      const newLine = this.codeContainer.querySelector(
        `[data-line="${lineNumber}"]`,
      );
      if (newLine) {
        newLine.classList.add("highlighted");
      }
    }

    reset() {
      this.highlightLine(null);
      this.codeContainer.scrollTop = 0;
    }

    destroy() {
      this.element.remove();
    }
  }

  // ============================================================================
  // Stack Frame Component
  // ============================================================================

  class DOMStackFrame {
    constructor(functionName, lineno, args = null) {
      this.functionName = functionName;
      this.lineno = lineno;
      this.args = args;
      this.isActive = false;
      this.color = getFunctionColor(functionName);

      this.element = document.createElement("div");
      this.element.className = "stack-frame";
      this.element.dataset.function = functionName;

      this.bgElement = document.createElement("div");
      this.bgElement.className = "stack-frame-bg";
      this.bgElement.style.backgroundColor = this.color;
      this.element.appendChild(this.bgElement);

      this.textElement = document.createElement("span");
      this.textElement.className = "stack-frame-text";
      this.textElement.textContent = functionName;
      this.element.appendChild(this.textElement);

      this.flashElement = document.createElement("div");
      this.flashElement.className = "stack-frame-flash";
      this.element.appendChild(this.flashElement);

      this.element.addEventListener("pointerover", this._onHover.bind(this));
      this.element.addEventListener("pointerout", this._onHoverOut.bind(this));
    }

    destroy() {
      this.element.parentNode?.removeChild(this.element);
    }

    updateLine(lineno) {
      this.lineno = lineno;
      this.textElement.textContent = this.functionName;
    }

    setActive(isActive) {
      if (this.isActive === isActive) return;
      this.isActive = isActive;
      this.bgElement.style.opacity = isActive ? "1.0" : "0.9";
    }

    _onHover() {
      this.bgElement.style.opacity = "0.8";
    }

    _onHoverOut() {
      this.bgElement.style.opacity = this.isActive ? "1.0" : "0.9";
    }

    flash(duration = 150) {
      this.flashElement.animate([{ opacity: 1 }, { opacity: 0 }], {
        duration,
        easing: "ease-out",
      });
    }

    getPosition() {
      const rect = this.element.getBoundingClientRect();
      return { x: rect.left, y: rect.top };
    }
  }

  // ============================================================================
  // Stack Visualization Component
  // ============================================================================

  class DOMStackVisualization {
    constructor() {
      this.frames = [];
      this.frameSpacing = LAYOUT.frameSpacing;

      this.element = document.createElement("div");
      this.element.className = "stack-visualization";
    }

    processEvent(event) {
      if (event.type === "call") {
        this.pushFrame(event.functionName, event.lineno, event.args);
      } else if (event.type === "return") {
        this.popFrame();
      } else if (event.type === "line") {
        this.updateTopFrameLine(event.lineno);
      }
    }

    updateTopFrameLine(lineno) {
      if (this.frames.length > 0) {
        this.frames[this.frames.length - 1].updateLine(lineno);
      }
    }

    pushFrame(functionName, lineno, args = null) {
      if (this.frames.length > 0) {
        this.frames[this.frames.length - 1].setActive(false);
      }

      const frame = new DOMStackFrame(functionName, lineno, args);
      frame.setActive(true);
      this.element.appendChild(frame.element);
      this.frames.push(frame);

      requestAnimationFrame(() => {
        frame.element.classList.add("visible");
      });
    }

    popFrame() {
      if (this.frames.length === 0) return;

      const frame = this.frames.pop();
      frame.element.classList.remove("visible");
      setTimeout(() => frame.destroy(), 300);

      if (this.frames.length > 0) {
        this.frames[this.frames.length - 1].setActive(true);
      }
    }

    clear() {
      this.frames.forEach((frame) => frame.destroy());
      this.frames = [];
      this.element.innerHTML = "";
    }

    flashAll() {
      this.frames.forEach((frame) => frame.flash());
    }

    createStackClone(container) {
      const clone = this.element.cloneNode(false);
      clone.className = "stack-visualization flying-clone";

      const elementRect = this.element.getBoundingClientRect();
      const containerRect = container.getBoundingClientRect();

      // Position relative to container since contain: strict makes position:fixed relative to container
      clone.style.position = "absolute";
      clone.style.left = elementRect.left - containerRect.left + "px";
      clone.style.top = elementRect.top - containerRect.top + "px";
      clone.style.width = elementRect.width + "px";
      clone.style.pointerEvents = "none";
      clone.style.zIndex = "1000";

      this.frames.forEach((frame) => {
        const frameClone = frame.element.cloneNode(true);
        frameClone.classList.add("visible");
        frameClone.style.opacity = "1";
        frameClone.style.transform = "translateY(0)";
        frameClone.style.transition = "none";
        clone.appendChild(frameClone);
      });

      container.appendChild(clone);
      return clone;
    }

    updateToMatch(targetStack) {
      while (this.frames.length > targetStack.length) {
        this.popFrame();
      }

      targetStack.forEach(({ func, line, args }, index) => {
        if (index < this.frames.length) {
          const frame = this.frames[index];
          if (frame.functionName !== func) {
            frame.updateLine(line);
          }
          frame.setActive(index === targetStack.length - 1);
        } else {
          this.pushFrame(func, line, args);
        }
      });

      if (this.frames.length > 0) {
        this.frames[this.frames.length - 1].setActive(true);
      }
    }
  }

  // ============================================================================
  // Sampling Panel Component
  // ============================================================================

  class DOMSamplingPanel {
    constructor() {
      this.samples = [];
      this.functionCounts = {};
      this.totalSamples = 0;
      this.sampleInterval = TIMINGS.sampleIntervalDefault;
      this.groundTruthFunctions = new Set();
      this.bars = {};

      this.element = document.createElement("div");
      this.element.className = "sampling-panel";

      const header = document.createElement("div");
      header.className = "sampling-header";

      const title = document.createElement("h3");
      title.className = "sampling-title";
      title.textContent = "Sampling Profiler";
      header.appendChild(title);

      const stats = document.createElement("div");
      stats.className = "sampling-stats";

      this.sampleCountEl = document.createElement("span");
      this.sampleCountEl.textContent = "Samples: 0";
      stats.appendChild(this.sampleCountEl);

      this.intervalEl = document.createElement("span");
      this.intervalEl.textContent = `Interval: ${this.sampleInterval}ms`;
      stats.appendChild(this.intervalEl);

      this.missedFunctionsEl = document.createElement("span");
      this.missedFunctionsEl.className = "missed";
      stats.appendChild(this.missedFunctionsEl);

      header.appendChild(stats);
      this.element.appendChild(header);

      this.barsContainer = document.createElement("div");
      this.barsContainer.className = "sampling-bars";
      this.element.appendChild(this.barsContainer);
    }

    setSampleInterval(interval) {
      this.sampleInterval = interval;
      this.intervalEl.textContent = `Interval: ${interval}ms`;
    }

    setGroundTruth(allFunctions) {
      this.groundTruthFunctions = new Set(allFunctions);
      this._updateMissedCount();
    }

    addSample(stack) {
      this.totalSamples++;
      this.sampleCountEl.textContent = `Samples: ${this.totalSamples}`;

      stack.forEach((frame) => {
        const funcName = frame.func;
        this.functionCounts[funcName] =
          (this.functionCounts[funcName] || 0) + 1;
      });

      this._updateBars();
      this._updateMissedCount();
    }

    reset() {
      this.samples = [];
      this.functionCounts = {};
      this.totalSamples = 0;
      this.sampleCountEl.textContent = "Samples: 0";
      this.missedFunctionsEl.textContent = "";
      this.barsContainer.innerHTML = "";
      this.bars = {};
    }

    _updateMissedCount() {
      if (this.groundTruthFunctions.size === 0) return;

      const capturedFunctions = new Set(Object.keys(this.functionCounts));
      const notYetSeen = [...this.groundTruthFunctions].filter(
        (f) => !capturedFunctions.has(f),
      );

      if (notYetSeen.length > 0) {
        this.missedFunctionsEl.textContent = `Not yet seen: ${notYetSeen.length}`;
        this.missedFunctionsEl.classList.add("missed");
        this.missedFunctionsEl.style.color = "";
      } else if (this.totalSamples > 0) {
        this.missedFunctionsEl.textContent = "All captured!";
        this.missedFunctionsEl.classList.remove("missed");
        this.missedFunctionsEl.style.color = "var(--color-green)";
      } else {
        this.missedFunctionsEl.textContent = "";
      }
    }

    _updateBars() {
      const sorted = Object.entries(this.functionCounts).sort(
        (a, b) => b[1] - a[1],
      );

      sorted.forEach(([funcName, count], index) => {
        const percentage =
          this.totalSamples > 0 ? count / this.totalSamples : 0;

        if (!this.bars[funcName]) {
          const row = this._createBarRow(funcName);
          this.barsContainer.appendChild(row);
          this.bars[funcName] = row;
        }

        const row = this.bars[funcName];
        const barFill = row.querySelector(".bar-fill");
        barFill.style.width = `${percentage * 100}%`;

        const percentEl = row.querySelector(".bar-percent");
        percentEl.textContent = `${(percentage * 100).toFixed(0)}%`;

        const currentIndex = Array.from(this.barsContainer.children).indexOf(
          row,
        );
        if (currentIndex !== index) {
          this.barsContainer.insertBefore(
            row,
            this.barsContainer.children[index],
          );
        }
      });
    }

    _createBarRow(funcName) {
      const row = document.createElement("div");
      row.className = "sampling-bar-row";
      row.dataset.function = funcName;

      const label = document.createElement("span");
      label.className = "bar-label";
      label.textContent = funcName;
      row.appendChild(label);

      const barContainer = document.createElement("div");
      barContainer.className = "bar-container";

      const barFill = document.createElement("div");
      barFill.className = "bar-fill";
      barFill.style.backgroundColor = getFunctionColor(funcName);
      barContainer.appendChild(barFill);

      row.appendChild(barContainer);

      const percent = document.createElement("span");
      percent.className = "bar-percent";
      percent.textContent = "0%";
      row.appendChild(percent);

      return row;
    }

    getTargetPosition() {
      const rect = this.barsContainer.getBoundingClientRect();
      return { x: rect.left + rect.width / 2, y: rect.top + 50 };
    }

    showImpactEffect(position) {
      const impact = document.createElement("div");
      impact.className = "impact-circle";
      impact.style.position = "fixed";
      impact.style.left = `${position.x}px`;
      impact.style.top = `${position.y}px`;

      // Append to barsContainer parent to avoid triggering scroll
      this.element.appendChild(impact);

      impact.animate(
        [
          { transform: "translate(-50%, -50%) scale(1)", opacity: 0.6 },
          { transform: "translate(-50%, -50%) scale(4)", opacity: 0 },
        ],
        {
          duration: 300,
          easing: "ease-out",
        },
      ).onfinish = () => impact.remove();
    }
  }

  // ============================================================================
  // Control Panel Component
  // ============================================================================

  class ControlPanel {
    constructor(
      container,
      onPlay,
      onPause,
      onReset,
      onSpeedChange,
      onSeek,
      onStep,
      onSampleIntervalChange = null,
    ) {
      this.container = container;
      this.onPlay = onPlay;
      this.onPause = onPause;
      this.onReset = onReset;
      this.onSpeedChange = onSpeedChange;
      this.onSeek = onSeek;
      this.onStep = onStep;
      this.onSampleIntervalChange = onSampleIntervalChange;

      this.isPlaying = false;
      this.speed = TIMINGS.defaultSpeed;

      this._createControls();
    }

    _createControls() {
      const panel = document.createElement("div");
      panel.id = "control-panel";

      const sampleIntervalHtml = this.onSampleIntervalChange
        ? `
        <div class="control-group">
          <label>Sample interval:</label>
          <input type="range" id="sample-interval"
                 min="${TIMINGS.sampleIntervalMin}"
                 max="${TIMINGS.sampleIntervalMax}"
                 value="${TIMINGS.sampleIntervalDefault}"
                 step="100"
                 aria-label="Sample interval in milliseconds">
          <span id="interval-display">${TIMINGS.sampleIntervalDefault}ms</span>
        </div>
      `
        : "";

      panel.innerHTML = `
        <div class="control-group">
          <button id="play-pause-btn" class="control-btn" aria-label="Play animation">▶ Play</button>
          <button id="reset-btn" class="control-btn" aria-label="Reset visualization to beginning">↻ Reset</button>
          <button id="step-btn" class="control-btn" aria-label="Step to next event">→ Step</button>
        </div>

        ${sampleIntervalHtml}

        <div class="control-group timeline-scrubber">
          <input type="range" id="timeline-scrubber" min="0" max="100" value="0" step="0.1" aria-label="Timeline position">
          <span id="time-display">0ms</span>
        </div>
      `;

      this.container.appendChild(panel);

      this.playPauseBtn = panel.querySelector("#play-pause-btn");
      this.resetBtn = panel.querySelector("#reset-btn");
      this.stepBtn = panel.querySelector("#step-btn");
      this.scrubber = panel.querySelector("#timeline-scrubber");
      this.timeDisplay = panel.querySelector("#time-display");

      this.playPauseBtn.addEventListener("click", () =>
        this._togglePlayPause(),
      );
      this.resetBtn.addEventListener("click", () => this._handleReset());
      this.stepBtn.addEventListener("click", () => this._handleStep());
      this.scrubber.addEventListener("input", (e) => this._handleSeek(e));

      if (this.onSampleIntervalChange) {
        this.sampleIntervalSlider = panel.querySelector("#sample-interval");
        this.intervalDisplay = panel.querySelector("#interval-display");
        this.sampleIntervalSlider.addEventListener("input", (e) =>
          this._handleSampleIntervalChange(e),
        );
      }
    }

    _handleSampleIntervalChange(e) {
      const interval = parseInt(e.target.value);
      this.intervalDisplay.textContent = `${interval}ms`;
      this.onSampleIntervalChange(interval);
    }

    _togglePlayPause() {
      this.isPlaying = !this.isPlaying;

      if (this.isPlaying) {
        this.playPauseBtn.textContent = "⏸ Pause";
        this.playPauseBtn.classList.add("active");
        this.onPlay();
      } else {
        this.playPauseBtn.textContent = "▶ Play";
        this.playPauseBtn.classList.remove("active");
        this.onPause();
      }
    }

    _handleReset() {
      this.isPlaying = false;
      this.playPauseBtn.textContent = "▶ Play";
      this.playPauseBtn.classList.remove("active");
      this.scrubber.value = 0;
      this.timeDisplay.textContent = "0ms";
      this.onReset();
    }

    _handleStep() {
      if (this.onStep) this.onStep();
    }

    _handleSeek(e) {
      const percentage = parseFloat(e.target.value);
      this.onSeek(percentage / 100);
    }

    updateTimeDisplay(currentTime, totalTime) {
      this.timeDisplay.textContent = `${Math.floor(currentTime)}ms / ${Math.floor(totalTime)}ms`;
      const percentage = (currentTime / totalTime) * 100;
      this.scrubber.value = percentage;
    }

    setDuration(duration) {
      this.duration = duration;
    }

    pause() {
      if (this.isPlaying) this._togglePlayPause();
    }

    destroy() {
      const panel = this.container.querySelector("#control-panel");
      if (panel) panel.remove();
    }
  }

  // ============================================================================
  // Visual Effects Manager
  // ============================================================================

  class VisualEffectsManager {
    constructor(container) {
      this.container = container;
      this.flyingAnimationInProgress = false;

      this.flashOverlay = document.createElement("div");
      this.flashOverlay.className = "flash-overlay";
      this.container.appendChild(this.flashOverlay);
    }

    triggerSamplingEffect(stackViz, samplingPanel, currentTime, trace) {
      if (this.flyingAnimationInProgress) return;

      const stack = trace.getStackAt(currentTime);

      if (stack.length === 0) {
        samplingPanel.addSample(stack);
        return;
      }

      this.flyingAnimationInProgress = true;
      stackViz.flashAll();

      const clone = stackViz.createStackClone(this.container);
      const targetPosition = samplingPanel.getTargetPosition();

      this._animateFlash();
      this._animateFlyingStack(clone, targetPosition, () => {
        samplingPanel.showImpactEffect(targetPosition);
        clone.remove();

        const currentStack = trace.getStackAt(currentTime);
        samplingPanel.addSample(currentStack);
        this.flyingAnimationInProgress = false;
      });
    }

    _animateFlash() {
      anim.to(this.flashOverlay, { opacity: 0.1 }, 0).onfinish = () => {
        anim.to(this.flashOverlay, { opacity: 0 }, 150, "easeOutQuad");
      };
    }

    _animateFlyingStack(clone, targetPosition, onComplete) {
      const containerRect = this.container.getBoundingClientRect();
      const cloneRect = clone.getBoundingClientRect();

      // Convert viewport coordinates to container-relative
      const startX = cloneRect.left - containerRect.left + cloneRect.width / 2;
      const startY = cloneRect.top - containerRect.top + cloneRect.height / 2;
      const targetX = targetPosition.x - containerRect.left;
      const targetY = targetPosition.y - containerRect.top;

      const deltaX = targetX - startX;
      const deltaY = targetY - startY;

      anim.to(
        clone,
        {
          x: deltaX,
          y: deltaY,
          scale: 0.3,
          opacity: 0.6,
        },
        TIMINGS.sampleToFlame,
        "easeOutCubic",
        onComplete,
      );
    }
  }

  // ============================================================================
  // Main Visualization Class
  // ============================================================================

  class SamplingVisualization {
    constructor(container) {
      this.container = container;

      this.trace = new ExecutionTrace(DEMO_SIMPLE.source, DEMO_SIMPLE.trace);

      this.currentTime = 0;
      this.isPlaying = false;
      this.playbackSpeed = TIMINGS.defaultSpeed;
      this.eventIndex = 0;

      this.sampleInterval = TIMINGS.sampleIntervalDefault;
      this.lastSampleTime = 0;

      this._createLayout();

      this.effectsManager = new VisualEffectsManager(this.vizColumn);

      this.lastTime = performance.now();
      this._animate();
    }

    _createLayout() {
      this.codePanel = new CodePanel(this.trace.source);
      this.container.appendChild(this.codePanel.element);

      this.vizColumn = document.createElement("div");
      this.vizColumn.className = "viz-column";
      this.container.appendChild(this.vizColumn);

      const stackSection = document.createElement("div");
      stackSection.className = "stack-section";

      const stackTitle = document.createElement("div");
      stackTitle.className = "stack-section-title";
      stackTitle.textContent = "Call Stack";
      stackSection.appendChild(stackTitle);

      this.stackViz = new DOMStackVisualization();
      stackSection.appendChild(this.stackViz.element);
      this.vizColumn.appendChild(stackSection);

      this.samplingPanel = new DOMSamplingPanel();
      this.samplingPanel.setGroundTruth(this._getGroundTruthFunctions());
      this.vizColumn.appendChild(this.samplingPanel.element);

      this.controls = new ControlPanel(
        this.vizColumn,
        () => this.play(),
        () => this.pause(),
        () => this.reset(),
        (speed) => this.setSpeed(speed),
        (progress) => this.seek(progress),
        () => this.step(),
        (interval) => this.setSampleInterval(interval),
      );
      this.controls.setDuration(this.trace.duration);
    }

    _getGroundTruthFunctions() {
      const functions = new Set();
      this.trace.events.forEach((event) => {
        if (event.type === "call") {
          functions.add(event.functionName);
        }
      });
      return [...functions];
    }

    play() {
      this.isPlaying = true;
    }

    pause() {
      this.isPlaying = false;
    }

    reset() {
      this.currentTime = 0;
      this.eventIndex = 0;
      this.isPlaying = false;
      this.lastSampleTime = 0;
      this.stackViz.clear();
      this.codePanel.reset();
      this.samplingPanel.reset();
      this.controls.updateTimeDisplay(0, this.trace.duration);
    }

    setSpeed(speed) {
      this.playbackSpeed = speed;
    }

    setSampleInterval(interval) {
      this.sampleInterval = interval;
      this.samplingPanel.setSampleInterval(interval);
    }

    seek(progress) {
      this.currentTime = progress * this.trace.duration;
      this.eventIndex = 0;
      this.lastSampleTime = 0;
      this._rebuildState();
    }

    step() {
      this.pause();

      const nextEvent = this.trace.getNextEvent(this.currentTime);

      if (nextEvent) {
        // Calculate delta to reach next event + epsilon
        const targetTime = nextEvent.timestamp + 0.1;
        const delta = targetTime - this.currentTime;
        if (delta > 0) {
          this._advanceTime(delta);
        }
      }
    }

    _animate(currentTime = performance.now()) {
      const deltaTime = currentTime - this.lastTime;
      this.lastTime = currentTime;

      this.update(deltaTime);
      requestAnimationFrame((t) => this._animate(t));
    }

    update(deltaTime) {
      if (!this.isPlaying) {
        this.controls.updateTimeDisplay(this.currentTime, this.trace.duration);
        return;
      }

      const virtualDelta = deltaTime * this.playbackSpeed;
      this._advanceTime(virtualDelta);
    }

    _advanceTime(virtualDelta) {
      this.currentTime += virtualDelta;

      if (this.currentTime >= this.trace.duration) {
        this.currentTime = this.trace.duration;
        this.isPlaying = false;
        this.controls.pause();
      }

      while (this.eventIndex < this.trace.events.length) {
        const event = this.trace.events[this.eventIndex];

        if (event.timestamp > this.currentTime) break;

        this._processEvent(event);
        this.eventIndex++;
      }

      this.controls.updateTimeDisplay(this.currentTime, this.trace.duration);

      if (this.currentTime - this.lastSampleTime >= this.sampleInterval) {
        this._takeSample();
        this.lastSampleTime = this.currentTime;
      }
    }

    _processEvent(event) {
      this.stackViz.processEvent(event);

      if (event.type === "call") {
        this.codePanel.highlightLine(event.lineno);
      } else if (event.type === "return") {
        const currentStack = this.trace.getStackAt(this.currentTime);
        if (currentStack.length > 0) {
          this.codePanel.highlightLine(
            currentStack[currentStack.length - 1].line,
          );
        } else {
          this.codePanel.highlightLine(null);
        }
      } else if (event.type === "line") {
        this.codePanel.highlightLine(event.lineno);
      }
    }

    _takeSample() {
      this.effectsManager.triggerSamplingEffect(
        this.stackViz,
        this.samplingPanel,
        this.currentTime,
        this.trace,
      );
    }

    _rebuildState() {
      this.stackViz.clear();
      this.codePanel.reset();
      this.samplingPanel.reset();

      for (let t = 0; t < this.currentTime; t += this.sampleInterval) {
        const stack = this.trace.getStackAt(t);
        this.samplingPanel.addSample(stack);
        this.lastSampleTime = t;
      }

      const stack = this.trace.getStackAt(this.currentTime);
      this.stackViz.updateToMatch(stack);

      if (stack.length > 0) {
        this.codePanel.highlightLine(stack[stack.length - 1].line);
      }

      this.eventIndex = this.trace.getEventsUntil(this.currentTime).length;
    }
  }

  // ============================================================================
  // Initialize
  // ============================================================================

  function init() {
    // If trace data hasn't been injected yet (local dev), don't initialize
    if (!DEMO_SIMPLE) return;

    const appContainer = document.getElementById("sampling-profiler-viz");
    if (appContainer) {
      new SamplingVisualization(appContainer);
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
