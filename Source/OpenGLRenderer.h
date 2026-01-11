/*
  ==============================================================================
    OpenGLRenderer.h - 3D stereo spectrum visualization with tracers
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SharedDataManager.h"
#include <vector>
#include <array>
#include <cmath>

namespace Colors
{
    constexpr uint32_t bg1 = 0xFF0d1117;
    constexpr uint32_t grid = 0xFF30363d;
    constexpr uint32_t gridBright = 0xFF505860;
    constexpr uint32_t text = 0xFFa0a8b0;
    constexpr uint32_t accent = 0xFF58a6ff;
    constexpr uint32_t warning = 0xFFff6b6b;
}

enum class ViewMode { Perspective3D, TopFlat, SideFlat };

struct Vtx { float x, y, z, r, g, b, a; };

// Store history of stereo positions for tracer effect
struct BandHistory
{
    static constexpr int kHistorySize = 8;
    std::array<float, kHistorySize> positions{};  // X positions
    int writeIndex = 0;
    
    void push(float x)
    {
        positions[static_cast<size_t>(writeIndex)] = x;
        writeIndex = (writeIndex + 1) % kHistorySize;
    }
    
    float get(int age) const  // age 0 = newest, age kHistorySize-1 = oldest
    {
        int idx = (writeIndex - 1 - age + kHistorySize * 2) % kHistorySize;
        return positions[static_cast<size_t>(idx)];
    }
};

class Spectral3DRenderer : public juce::Component,
                           public juce::OpenGLRenderer,
                           private juce::Timer
{
public:
// --- Unified Camera Constants ---
    static constexpr float defaultRotX = 40.0f;
    static constexpr float defaultRotY = 180.0f;
    static constexpr float defaultZoom = 3.8f;

    explicit Spectral3DRenderer(juce::SharedResourcePointer<SharedDataManager>& data,
                                std::atomic<float>* rangeParam = nullptr)
        : sharedData(data), rangePtr(rangeParam)
    {
        // Use unified constants for initialization
        rotX = defaultRotX;
        rotY = defaultRotY;
        zoom = defaultZoom;

        ctx.setRenderer(this);
        ctx.setContinuousRepainting(false);
        ctx.setComponentPaintingEnabled(true);
        ctx.attachTo(*this);
        
        // Initialize history for all possible tracks and bands
        for (auto& trackHist : bandHistories)
            for (auto& hist : trackHist)
                for (auto& p : hist.positions)
                    p = 0.0f;
        
        startTimerHz(30);
    }
    
    ~Spectral3DRenderer() override
    {
        stopTimer();
        ctx.detach();
    }
    
    void setViewMode(ViewMode m) { viewMode = m; repaint(); }
    ViewMode getViewMode() const { return viewMode; }

    void resetView() { 
        rotX = defaultRotX; 
        rotY = defaultRotY; 
        zoom = defaultZoom;
        // Ensure the next mouse drag starts from these exact coordinates
        // This prevents the camera from "jumping" back to a previous state
        repaint();
    }
    
    void newOpenGLContextCreated() override { buildShader(); }
    
    void renderOpenGL() override
    {
        using namespace juce::gl;
        
        if (!juce::OpenGLHelpers::isContextActive()) return;
        
        float scale = static_cast<float>(ctx.getRenderingScale());
        int w = juce::roundToInt(scale * static_cast<float>(getWidth()));
        int h = juce::roundToInt(scale * static_cast<float>(getHeight()));
        if (w <= 0 || h <= 0) return;
        
        glViewport(0, 0, w, h);
        
        auto bgCol = juce::Colour(Colors::bg1);
        glClearColor(bgCol.getFloatRed(), bgCol.getFloatGreen(), bgCol.getFloatBlue(), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        
        if (shader != nullptr && shader->getProgramID() != 0)
        {
            shader->use();
            
            auto proj = makeProj(static_cast<float>(getWidth()), static_cast<float>(getHeight()));
            auto view = makeView();
            
            if (uProj != nullptr) uProj->setMatrix4(proj.data(), 1, false);
            if (uView != nullptr) uView->setMatrix4(view.data(), 1, false);
            
            buildGeometry();
            
            if (!lineVerts.empty() || !triVerts.empty())
                drawVerts();
        }
        
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
    }
    
    void openGLContextClosing() override
    {
        shader.reset();
        if (lineVbo != 0) { juce::gl::glDeleteBuffers(1, &lineVbo); lineVbo = 0; }
        if (triVbo != 0) { juce::gl::glDeleteBuffers(1, &triVbo); triVbo = 0; }
    }
    
    void paint(juce::Graphics&) override
    {
        // Labels removed
    }
    
    void mouseDown(const juce::MouseEvent& e) override { lastMouse = e.position; }
    
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (viewMode != ViewMode::Perspective3D) return;
        auto d = e.position - lastMouse;
        rotY += d.x * 0.4f;
        rotX = juce::jlimit(-89.0f, 89.0f, rotX + d.y * 0.4f);
        lastMouse = e.position;
    }
    
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wh) override
    {
        if (viewMode != ViewMode::Perspective3D) return;
        zoom = juce::jlimit(1.5f, 6.0f, zoom - wh.deltaY * 0.3f);
    }
    
private:
    void timerCallback() override
    {
        sharedData->cleanupStale(1000);
        ctx.triggerRepaint();
    }
    
    void buildShader()
    {
        const char* vs = R"(
            attribute vec3 aPos;
            attribute vec4 aCol;
            uniform mat4 uProj, uView;
            varying vec4 vCol;
            void main() {
                vCol = aCol;
                gl_Position = uProj * uView * vec4(aPos, 1.0);
            })";
        
        const char* fs = R"(
            varying vec4 vCol;
            void main() { gl_FragColor = vCol; })";
        
        auto s = std::make_unique<juce::OpenGLShaderProgram>(ctx);
        if (s->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(vs))
            && s->addFragmentShader(juce::OpenGLHelpers::translateFragmentShaderToV3(fs))
            && s->link())
        {
            shader = std::move(s);
            uProj = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader, "uProj");
            uView = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader, "uView");
            aPos = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "aPos");
            aCol = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader, "aCol");
        }
    }
    
    std::array<float, 16> makeProj(float w, float h)
    {
        std::array<float, 16> m = {};
        if (viewMode == ViewMode::Perspective3D)
        {
            float asp = w / h, fov = 50.0f, n = 0.1f, f = 50.0f;
            float t = std::tan(juce::degreesToRadians(fov) / 2.0f);
            m[0] = -1.0f / (asp * t); // Negative for right-handed coord system
            m[5] = 1.0f / t;
            m[10] = -(f + n) / (f - n);
            m[11] = -1.0f;
            m[14] = -2.0f * f * n / (f - n);
        }
        else
        {
            float asp = w / h;
            float size = 1.25f;
            m[0] = 1.0f / (size * asp);
            m[5] = 1.0f / size;
            m[10] = -0.02f;
            m[15] = 1.0f;
        }
        return m;
    }
    
    std::array<float, 16> makeView()
    {
        std::array<float, 16> m = {};
        
        if (viewMode == ViewMode::TopFlat)
        {
            // Looking down from above, rotated so Z=-1 (low freq) at bottom of screen, Z=+1 (high freq) at top
            m[0] = 1;   m[1] = 0;  m[2] = 0;   m[3] = 0;
            m[4] = 0;   m[5] = 0;  m[6] = 1;   m[7] = 0;
            m[8] = 0;   m[9] = 1;  m[10] = 0;  m[11] = 0;
            m[12] = 0;  m[13] = 0; m[14] = -3.0f; m[15] = 1;
            return m;
        }
        else if (viewMode == ViewMode::SideFlat)
        {
            m[0] = 0; m[2] = -1; m[5] = 1; m[8] = 1; m[10] = 0;
            m[14] = -3.0f; m[15] = 1;
            return m;
        }
        
        float rx = juce::degreesToRadians(rotX);
        float ry = juce::degreesToRadians(rotY);
        float cx = std::cos(rx), sx = std::sin(rx);
        float cy = std::cos(ry), sy = std::sin(ry);
        
        float camX = zoom * cx * sy;
        float camY = zoom * sx;
        float camZ = zoom * cx * cy;
        
        float fx = -camX, fy = -camY, fz = -camZ;
        float fl = std::sqrt(fx*fx + fy*fy + fz*fz);
        if (fl > 0) { fx /= fl; fy /= fl; fz /= fl; }
        
        float rx_ = fy * 0 - fz * 1, ry_ = fz * 0 - fx * 0, rz_ = fx * 1 - fy * 0;
        float rl = std::sqrt(rx_*rx_ + ry_*ry_ + rz_*rz_);
        if (rl > 0) { rx_ /= rl; ry_ /= rl; rz_ /= rl; }
        
        float ux_ = ry_*fz - rz_*fy, uy_ = rz_*fx - rx_*fz, uz_ = rx_*fy - ry_*fx;
        
        m[0] = rx_;  m[4] = ry_;  m[8]  = rz_;  m[12] = -(rx_*camX + ry_*camY + rz_*camZ);
        m[1] = ux_;  m[5] = uy_;  m[9]  = uz_;  m[13] = -(ux_*camX + uy_*camY + uz_*camZ);
        m[2] = -fx;  m[6] = -fy;  m[10] = -fz;  m[14] = fx*camX + fy*camY + fz*camZ;
        m[3] = 0;    m[7] = 0;    m[11] = 0;    m[15] = 1;
        return m;
    }
    
    void buildGeometry()
    {
        lineVerts.clear();
        triVerts.clear();
        lineVerts.reserve(3000);
        triVerts.reserve(10000);
        
        addGrid();
        addTracks();
    }
    
    void addLine(float x1, float y1, float z1, float x2, float y2, float z2,
                 float r, float g, float b, float a)
    {
        lineVerts.push_back({x1, y1, z1, r, g, b, a});
        lineVerts.push_back({x2, y2, z2, r, g, b, a});
    }
    
    void addTriangle(float x1, float y1, float z1,
                     float x2, float y2, float z2,
                     float x3, float y3, float z3,
                     float r, float g, float b, float a)
    {
        triVerts.push_back({x1, y1, z1, r, g, b, a});
        triVerts.push_back({x2, y2, z2, r, g, b, a});
        triVerts.push_back({x3, y3, z3, r, g, b, a});
    }
    
    void addQuad(float x1, float y1, float z1,
                 float x2, float y2, float z2,
                 float x3, float y3, float z3,
                 float x4, float y4, float z4,
                 float r, float g, float b, float a)
    {
        addTriangle(x1, y1, z1, x2, y2, z2, x3, y3, z3, r, g, b, a);
        addTriangle(x1, y1, z1, x3, y3, z3, x4, y4, z4, r, g, b, a);
    }
    
    void addGrid()
    {
        auto gc = juce::Colour(Colors::grid);
        float gr = gc.getFloatRed(), gg = gc.getFloatGreen(), gb = gc.getFloatBlue();
        
        for (int i = 0; i <= 4; ++i)
        {
            float t = static_cast<float>(i) / 4.0f;
            float p = -1.0f + t * 2.0f;
            addLine(p, -1, -1, p, -1, 1, gr, gg, gb, 0.4f);
            addLine(-1, -1, p, 1, -1, p, gr, gg, gb, 0.4f);
        }
        
        auto bc = juce::Colour(Colors::gridBright);
        float br = bc.getFloatRed(), bg_ = bc.getFloatGreen(), bb = bc.getFloatBlue();
        
        // Box edges
        addLine(-1, -1, -1, 1, -1, -1, br, bg_, bb, 0.6f);
        addLine(-1, -1, 1, 1, -1, 1, br, bg_, bb, 0.6f);
        addLine(-1, -1, -1, -1, -1, 1, br, bg_, bb, 0.6f);
        addLine(1, -1, -1, 1, -1, 1, br, bg_, bb, 0.6f);
        
        addLine(-1, -1, -1, -1, 1, -1, br, bg_, bb, 0.5f);
        addLine(1, -1, -1, 1, 1, -1, br, bg_, bb, 0.5f);
        addLine(-1, -1, 1, -1, 1, 1, br, bg_, bb, 0.5f);
        addLine(1, -1, 1, 1, 1, 1, br, bg_, bb, 0.5f);
        
        auto wc = juce::Colour(Colors::warning);
        float wr = wc.getFloatRed(), wg = wc.getFloatGreen(), wb = wc.getFloatBlue();
        addLine(-1, 1, -1, 1, 1, -1, wr, wg, wb, 0.5f);
        addLine(-1, 1, 1, 1, 1, 1, wr, wg, wb, 0.5f);
        addLine(-1, 1, -1, -1, 1, 1, wr, wg, wb, 0.5f);
        addLine(1, 1, -1, 1, 1, 1, wr, wg, wb, 0.5f);
        
        // Center line
        auto ac = juce::Colour(Colors::accent);
        addLine(0, -1, -1, 0, -1, 1, ac.getFloatRed(), ac.getFloatGreen(), ac.getFloatBlue(), 0.5f);
    }
    
    void addTracks()
    {
        float rangeVal = rangePtr != nullptr ? rangePtr->load() : 36.0f;
        
        auto levelToY = [rangeVal](float linearLevel) {
            if (linearLevel < 0.0001f) return -1.0f;
            float db = juce::Decibels::gainToDecibels(linearLevel, -100.0f);
            float normalized = (db + rangeVal) / rangeVal;
            return juce::jlimit(-1.0f, 1.0f, normalized * 2.0f - 1.0f);
        };
        
        for (size_t t = 0; t < kMaxTracks; ++t)
        {
            const auto& track = sharedData->getTrack(static_cast<int>(t));
            if (!track.isActive.load(std::memory_order_acquire)) continue;
            
            auto col = track.getColor();
            float cr = std::min(1.0f, col.getFloatRed() * 1.3f);
            float cg = std::min(1.0f, col.getFloatGreen() * 1.3f);
            float cb = std::min(1.0f, col.getFloatBlue() * 1.3f);
            
            int numBands = track.numBands.load(std::memory_order_relaxed);
            if (numBands < 1) numBands = 24;
            
            // Fixed width for all bands
            constexpr float bandWidth = 0.03f;
            
            for (int band = 0; band < numBands; ++band)
            {
                float left, right;
                track.getBand(static_cast<size_t>(band), left, right);
                
                float z = -1.0f + (static_cast<float>(band) + 0.5f) / static_cast<float>(numBands) * 2.0f;
                float ly = levelToY(left);
                float ry = levelToY(right);
                float avgY = (ly + ry) * 0.5f;
                
                if (avgY < -0.95f) continue;
                
                // Pan position: -1 = full left, +1 = full right
                float total = left + right + 0.0001f;
                float balance = (right - left) / total;
                float centerX = balance;  // Full range -1 to +1
                
                // Update history for this band
                auto& hist = bandHistories[t][static_cast<size_t>(band)];
                hist.push(centerX);
                
                float alpha = juce::jlimit(0.5f, 1.0f, avgY * 0.5f + 0.7f);
                
                // Draw tracer (fading history trail)
                for (int age = BandHistory::kHistorySize - 1; age >= 1; --age)
                {
                    float oldX = hist.get(age);
                    float newX = hist.get(age - 1);
                    
                    // Skip if no movement
                    if (std::abs(oldX - newX) < 0.001f) continue;
                   
                    // alpha * decay factor * base intensity
                    float tracerAlpha = alpha * (1.0f - static_cast<float>(age) / static_cast<float>(BandHistory::kHistorySize)) * 0.7f;
                    
                    // Draw tracer line connecting old and new positions
                    addLine(oldX, avgY, z, newX, avgY, z, cr, cg, cb, tracerAlpha);
                }
                
                // Draw current position bar (fixed width, clamped to box)
                float lx = juce::jlimit(-1.0f, 1.0f - bandWidth * 2, centerX - bandWidth);
                float rx = juce::jlimit(-1.0f + bandWidth * 2, 1.0f, centerX + bandWidth);
                
                // Filled bar from floor to amplitude
                addQuad(lx, -1.0f, z - 0.02f,
                       rx, -1.0f, z - 0.02f,
                       rx, avgY, z - 0.02f,
                       lx, avgY, z - 0.02f,
                       cr * 0.8f, cg * 0.8f, cb, alpha * 0.5f);
                
                addQuad(lx, -1.0f, z + 0.02f,
                       rx, -1.0f, z + 0.02f,
                       rx, avgY, z + 0.02f,
                       lx, avgY, z + 0.02f,
                       cr, cg * 0.8f, cb * 0.8f, alpha * 0.5f);
                
                // Top cap
                addQuad(lx, avgY, z - 0.02f,
                       rx, avgY, z - 0.02f,
                       rx, avgY, z + 0.02f,
                       lx, avgY, z + 0.02f,
                       cr, cg, cb, alpha * 0.4f);
                
                // Bright edge lines
                addLine(lx, -1.0f, z, lx, avgY, z, cr * 0.9f, cg, cb, alpha);
                addLine(rx, -1.0f, z, rx, avgY, z, cr, cg, cb * 0.9f, alpha);
                addLine(lx, avgY, z, rx, avgY, z, cr, cg, cb, alpha);
            }
            
            // Connect bands with lines
            float prevX = 0, prevY = -1, prevZ = -1;
            bool first = true;
            
            for (int band = 0; band < numBands; ++band)
            {
                float left, right;
                track.getBand(static_cast<size_t>(band), left, right);
                
                float z = -1.0f + (static_cast<float>(band) + 0.5f) / static_cast<float>(numBands) * 2.0f;
                float avgY = (levelToY(left) + levelToY(right)) * 0.5f;
                
                if (avgY < -0.95f) { first = true; continue; }
                
                float total = left + right + 0.0001f;
                float balance = (right - left) / total;
                float centerX = balance;  // Full range -1 to +1
                
                if (!first)
                {
                    addLine(prevX, prevY, prevZ, centerX, avgY, z, cr, cg, cb, 0.4f);
                }
                
                prevX = centerX;
                prevY = avgY;
                prevZ = z;
                first = false;
            }
        }
    }
    
    void drawVerts()
    {
        using namespace juce::gl;
        
        if (aPos == nullptr || aCol == nullptr) return;
        if (aPos->attributeID < 0 || aCol->attributeID < 0) return;
        
        GLuint pa = static_cast<GLuint>(aPos->attributeID);
        GLuint ca = static_cast<GLuint>(aCol->attributeID);
        
        // Draw triangles first
        if (!triVerts.empty())
        {
            if (triVbo == 0) glGenBuffers(1, &triVbo);
            
            glBindBuffer(GL_ARRAY_BUFFER, triVbo);
            glBufferData(GL_ARRAY_BUFFER, 
                        static_cast<GLsizeiptr>(triVerts.size() * sizeof(Vtx)), 
                        triVerts.data(), GL_STREAM_DRAW);
            
            glEnableVertexAttribArray(pa);
            glEnableVertexAttribArray(ca);
            
            glVertexAttribPointer(pa, 3, GL_FLOAT, GL_FALSE, sizeof(Vtx), nullptr);
            glVertexAttribPointer(ca, 4, GL_FLOAT, GL_FALSE, sizeof(Vtx), 
                                reinterpret_cast<void*>(3 * sizeof(float)));
            
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triVerts.size()));
            
            glDisableVertexAttribArray(pa);
            glDisableVertexAttribArray(ca);
        }
        
        // Draw lines on top
        if (!lineVerts.empty())
        {
            if (lineVbo == 0) glGenBuffers(1, &lineVbo);
            
            glBindBuffer(GL_ARRAY_BUFFER, lineVbo);
            glBufferData(GL_ARRAY_BUFFER, 
                        static_cast<GLsizeiptr>(lineVerts.size() * sizeof(Vtx)), 
                        lineVerts.data(), GL_STREAM_DRAW);
            
            glEnableVertexAttribArray(pa);
            glEnableVertexAttribArray(ca);
            
            glVertexAttribPointer(pa, 3, GL_FLOAT, GL_FALSE, sizeof(Vtx), nullptr);
            glVertexAttribPointer(ca, 4, GL_FLOAT, GL_FALSE, sizeof(Vtx), 
                                reinterpret_cast<void*>(3 * sizeof(float)));
            
            glLineWidth(2.0f);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVerts.size()));
            
            glDisableVertexAttribArray(pa);
            glDisableVertexAttribArray(ca);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    juce::SharedResourcePointer<SharedDataManager>& sharedData;
    std::atomic<float>* rangePtr = nullptr;
    juce::OpenGLContext ctx;
    
    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> uProj, uView;
    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> aPos, aCol;
    
    std::vector<Vtx> lineVerts;
    std::vector<Vtx> triVerts;
    GLuint lineVbo = 0;
    GLuint triVbo = 0;
    
    // History for tracer effect - per track, per band
    std::array<std::array<BandHistory, kMaxBands>, kMaxTracks> bandHistories;
    
    ViewMode viewMode = ViewMode::Perspective3D;
    float rotX = 25.0f, rotY = -35.0f, zoom = 2.8f;
    juce::Point<float> lastMouse;
};
