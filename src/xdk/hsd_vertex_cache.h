/* Included after the GX vertex decoder, inside the renderer namespace.
 * Cache only immutable descriptor geometry; animation matrices, materials
 * and texture routing are still evaluated on every draw. Shape meshes bypass
 * this cache. PObj removal invalidates entries before addresses can be reused. */
struct VertexCacheEntry {
    const HSD_PObj* owner;
    const HSD_VtxDescList* descs;
    const unsigned char* display;
    unsigned displayBlocks, stride, vertices, state;
    RawVertex* raw;
};
const unsigned kVertexCacheSlots = 1024;
const unsigned kVertexCacheBytes = 8 * 1024 * 1024;
const unsigned kRawVertexBytes = sizeof(RawVertex);
static VertexCacheEntry s_vertexCache[kVertexCacheSlots];
static unsigned s_vertexCacheBytes;

static unsigned VertexCacheHash(const HSD_PObj* owner)
{
    return (unsigned) ((uintptr_t) owner >> 5) % kVertexCacheSlots;
}

static void ReleaseVertexCacheEntry(VertexCacheEntry* entry)
{
    if (entry->state == 1) {
        s_vertexCacheBytes -= entry->vertices * kRawVertexBytes;
        free(entry->raw);
    }
    memset(entry, 0, sizeof(*entry));
    entry->state = 2; /* Preserve the probe chain after removal. */
}

static void ForgetVertexCache(const HSD_PObj* owner)
{
    const unsigned hash = VertexCacheHash(owner);
    for (unsigned probe = 0; probe < kVertexCacheSlots; ++probe) {
        VertexCacheEntry* entry = &s_vertexCache[(hash + probe) % kVertexCacheSlots];
        if (entry->state == 0) return;
        if (entry->state == 1 && entry->owner == owner) {
            ReleaseVertexCacheEntry(entry);
            return;
        }
    }
}

static void ClearVertexCache(void)
{
    for (unsigned i = 0; i < kVertexCacheSlots; ++i)
        if (s_vertexCache[i].state == 1) free(s_vertexCache[i].raw);
    memset(s_vertexCache, 0, sizeof(s_vertexCache));
    s_vertexCacheBytes = 0;
}

static unsigned VertexStreamStride(const HSD_VtxDescList* descs)
{
    unsigned bytes = 0, count = 0;
    for (const HSD_VtxDescList* d = descs;
         d && d->attr != GX_VA_NULL && count < 32; ++d, ++count) {
        if (d->attr_type == GX_NONE) continue;
        const unsigned indices = (d->attr == GX_VA_NRM || d->attr == GX_VA_NBT) &&
                                  d->comp_cnt == GX_NRM_NBT3 ? 3u : 1u;
        if (d->attr_type == GX_INDEX8) bytes += indices;
        else if (d->attr_type == GX_INDEX16) bytes += indices * 2;
        else if (d->attr_type == GX_DIRECT) bytes += DirectSize(d);
        else return 0;
    }
    return bytes;
}

static VertexCacheEntry* GetVertexCache(HSD_PObj* owner)
{
    if (pobj_type(owner) == POBJ_SHAPEANIM) {
        ForgetVertexCache(owner);
        return NULL;
    }
    if (!owner->verts || !owner->display || !owner->n_display || owner->n_display > 0x4000)
        return NULL;
    VertexCacheEntry* vacant = NULL;
    const unsigned hash = VertexCacheHash(owner);
    for (unsigned probe = 0; probe < kVertexCacheSlots; ++probe) {
        VertexCacheEntry* entry = &s_vertexCache[(hash + probe) % kVertexCacheSlots];
        if (entry->state == 1 && entry->owner == owner) {
            if (entry->descs == owner->verts && entry->display == owner->display &&
                entry->displayBlocks == owner->n_display) return entry;
            ReleaseVertexCacheEntry(entry);
        }
        if (entry->state != 1 && !vacant) vacant = entry;
        if (entry->state == 0) break;
    }
    if (!vacant) return NULL;
    const unsigned stride = VertexStreamStride(owner->verts);
    if (!stride) return NULL;
    const unsigned char* cursor = owner->display;
    const unsigned char* end = cursor + (static_cast<unsigned>(owner->n_display) << 5);
    unsigned vertices = 0;
    while (end - cursor >= 3) {
        if ((*cursor & 0xF8) == 0) break;
        const unsigned n = Read16(cursor + 1);
        cursor += 3;
        if (n > static_cast<unsigned>(end - cursor) / stride) return NULL;
        if (n > kVertexCacheBytes / kRawVertexBytes - vertices) return NULL;
        vertices += n;
        cursor += n * stride;
    }
    const unsigned bytes = vertices * kRawVertexBytes;
    if (!bytes || bytes > kVertexCacheBytes - s_vertexCacheBytes) return NULL;
    RawVertex* raw = static_cast<RawVertex*>(malloc(bytes));
    if (!raw) return NULL;
    cursor = owner->display;
    unsigned decoded = 0;
    while (end - cursor >= 3) {
        if ((*cursor & 0xF8) == 0) break;
        const unsigned n = Read16(cursor + 1);
        cursor += 3;
        for (unsigned i = 0; i < n; ++i) {
            if (!ParseVertex(owner->verts, &cursor, end, &raw[decoded++])) {
                free(raw);
                return NULL;
            }
        }
    }
    vacant->owner = owner;
    vacant->descs = owner->verts;
    vacant->display = owner->display;
    vacant->displayBlocks = owner->n_display;
    vacant->stride = stride;
    vacant->vertices = vertices;
    vacant->raw = raw;
    vacant->state = 1;
    s_vertexCacheBytes += bytes;
    return vacant;
}
