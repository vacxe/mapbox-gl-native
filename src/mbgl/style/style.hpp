#ifndef MBGL_STYLE_STYLE
#define MBGL_STYLE_STYLE

#include <mbgl/style/zoom_history.hpp>
#include <mbgl/style/property_transition.hpp>

#include <mbgl/source/source.hpp>
#include <mbgl/text/glyph_store.hpp>
#include <mbgl/sprite/sprite_store.hpp>
#include <mbgl/map/mode.hpp>

#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/chrono.hpp>
#include <mbgl/util/worker.hpp>
#include <mbgl/util/optional.hpp>
#include <mbgl/util/feature.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace mbgl {

class FileSource;
class GlyphAtlas;
class GlyphStore;
class SpriteStore;
class SpriteAtlas;
class LineAtlas;
class StyleLayer;
class Tile;
class Bucket;
class StyleUpdateParameters;
class StyleQueryParameters;

struct RenderItem {
    inline RenderItem(const StyleLayer& layer_,
                      const Tile* tile_ = nullptr,
                      Bucket* bucket_ = nullptr)
        : tile(tile_), bucket(bucket_), layer(layer_) {
    }

    const Tile* const tile;
    Bucket* const bucket;
    const StyleLayer& layer;
};

struct RenderData {
    Color backgroundColor = {{ 0, 0, 0, 0 }};
    std::set<Source*> sources;
    std::vector<RenderItem> order;
};

class Style : public GlyphStore::Observer,
              public SpriteStore::Observer,
              public Source::Observer,
              public util::noncopyable {
public:
    Style(FileSource&, float pixelRatio);
    ~Style();

    class Observer : public GlyphStore::Observer,
                     public SpriteStore::Observer,
                     public Source::Observer {
    public:
        /**
         * In addition to the individual glyph, sprite, and source events, the
         * following "rollup" events are provided for convenience. They are
         * strictly additive; e.g. when a source is loaded, both `onSourceLoaded`
         * and `onResourceLoaded` will be called.
         */
         virtual void onResourceLoaded() {};
         virtual void onResourceError(std::exception_ptr) {};
    };

    void setJSON(const std::string& data, const std::string& base);

    void setObserver(Observer*);

    bool isLoaded() const;

    // Fetch the tiles needed by the current viewport and emit a signal when
    // a tile is ready so observers can render the tile.
    void update(const StyleUpdateParameters&);

    void cascade(const TimePoint&, MapMode);
    void recalculate(float z, const TimePoint&, MapMode);

    bool hasTransitions() const;

    std::exception_ptr getLastError() const {
        return lastError;
    }

    Source* getSource(const std::string& id) const;
    void addSource(std::unique_ptr<Source>);

    std::vector<std::unique_ptr<StyleLayer>> getLayers() const;
    StyleLayer* getLayer(const std::string& id) const;
    void addLayer(std::unique_ptr<StyleLayer>,
                  optional<std::string> beforeLayerID = {});
    void removeLayer(const std::string& layerID);

    bool addClass(const std::string&, const PropertyTransition& = {});
    bool removeClass(const std::string&, const PropertyTransition& = {});
    bool hasClass(const std::string&) const;
    void setClasses(const std::vector<std::string>&, const PropertyTransition& = {});
    std::vector<std::string> getClasses() const;

    RenderData getRenderData() const;

    std::vector<Feature> queryRenderedFeatures(const StyleQueryParameters&) const;

    float getQueryRadius() const;

    void setSourceTileCacheSize(size_t);
    void onLowMemory();

    void dumpDebugLogs() const;

    FileSource& fileSource;
    std::unique_ptr<GlyphStore> glyphStore;
    std::unique_ptr<GlyphAtlas> glyphAtlas;
    std::unique_ptr<SpriteStore> spriteStore;
    std::unique_ptr<SpriteAtlas> spriteAtlas;
    std::unique_ptr<LineAtlas> lineAtlas;

private:
    std::vector<std::unique_ptr<Source>> sources;
    std::vector<std::unique_ptr<StyleLayer>> layers;
    std::vector<std::string> classes;
    optional<PropertyTransition> transitionProperties;

    std::vector<std::unique_ptr<StyleLayer>>::const_iterator findLayer(const std::string& layerID) const;

    // GlyphStore::Observer implementation.
    void onGlyphsLoaded(const FontStack&, const GlyphRange&) override;
    void onGlyphsError(const FontStack&, const GlyphRange&, std::exception_ptr) override;

    // SpriteStore::Observer implementation.
    void onSpriteLoaded() override;
    void onSpriteError(std::exception_ptr) override;

    // Source::Observer implementation.
    void onSourceLoaded(Source&) override;
    void onSourceError(Source&, std::exception_ptr) override;
    void onTileLoaded(Source&, const OverscaledTileID&, bool isNewTile) override;
    void onTileError(Source&, const OverscaledTileID&, std::exception_ptr) override;
    void onPlacementRedone() override;

    Observer nullObserver;
    Observer* observer = &nullObserver;

    std::exception_ptr lastError;

    ZoomHistory zoomHistory;
    bool hasPendingTransitions = false;

public:
    bool shouldReparsePartialTiles = false;
    bool loaded = false;
    Worker workers;
};

} // namespace mbgl

#endif
