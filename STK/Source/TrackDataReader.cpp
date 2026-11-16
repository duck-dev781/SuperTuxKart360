#include "TrackDataReader.h"

#include <cstdlib>
#include <map>
#include <sstream>

namespace stk360
{
    namespace
    {
        const unsigned int kMaxQuads = 10000;
        const unsigned int kMaxEdges = 50000;

        struct XmlTag
        {
            std::string name;
            std::map<std::string, std::string> attributes;
            bool closing;
        };

        void setError(std::string* error, const char* message)
        {
            if (error)
                *error = message;
        }

        bool parseTag(const char* xml, unsigned int size,
                      unsigned int* cursor, XmlTag* tag)
        {
            if (!xml || !cursor || !tag)
                return false;

            while (*cursor < size)
            {
                while (*cursor < size && xml[*cursor] != '<')
                    ++(*cursor);
                if (*cursor >= size)
                    return false;

                if (*cursor + 1 < size && xml[*cursor + 1] == '!')
                {
                    unsigned int end = *cursor + 2;
                    while (end + 2 < size &&
                           !(xml[end] == '-' && xml[end + 1] == '-' && xml[end + 2] == '>'))
                        ++end;
                    if (end + 2 >= size)
                    {
                        *cursor = size;
                        return false;
                    }
                    *cursor = end + 3;
                    continue;
                }

                if (*cursor + 1 < size && xml[*cursor + 1] == '?')
                {
                    unsigned int end = *cursor + 2;
                    while (end + 1 < size &&
                           !(xml[end] == '?' && xml[end + 1] == '>'))
                        ++end;
                    if (end + 1 >= size)
                    {
                        *cursor = size;
                        return false;
                    }
                    *cursor = end + 2;
                    continue;
                }

                unsigned int end = *cursor + 1;
                char quote = 0;
                while (end < size)
                {
                    const char c = xml[end];
                    if (quote)
                    {
                        if (c == quote)
                            quote = 0;
                    }
                    else if (c == '"' || c == 39)
                        quote = c;
                    else if (c == '>')
                        break;
                    ++end;
                }
                if (end >= size || quote)
                {
                    *cursor = size;
                    return false;
                }

                std::string body(xml + *cursor + 1, xml + end);
                *cursor = end + 1;
                tag->name.clear();
                tag->attributes.clear();
                tag->closing = false;

                unsigned int pos = 0;
                while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t' ||
                       body[pos] == '\r' || body[pos] == '\n'))
                    ++pos;
                if (pos < body.size() && body[pos] == '/')
                {
                    tag->closing = true;
                    ++pos;
                }

                const unsigned int name_start = pos;
                while (pos < body.size() && body[pos] != ' ' && body[pos] != '\t' &&
                       body[pos] != '\r' && body[pos] != '\n' && body[pos] != '/')
                    ++pos;
                if (pos == name_start)
                    continue;
                tag->name.assign(body, name_start, pos - name_start);

                while (pos < body.size())
                {
                    while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t' ||
                           body[pos] == '\r' || body[pos] == '\n' || body[pos] == '/'))
                        ++pos;
                    if (pos >= body.size())
                        break;

                    const unsigned int key_start = pos;
                    while (pos < body.size() && body[pos] != '=' && body[pos] != ' ' &&
                           body[pos] != '\t' && body[pos] != '\r' && body[pos] != '\n')
                        ++pos;
                    if (pos == key_start)
                        break;
                    const std::string key(body, key_start, pos - key_start);

                    while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t' ||
                           body[pos] == '\r' || body[pos] == '\n'))
                        ++pos;
                    if (pos >= body.size() || body[pos] != '=')
                        break;
                    ++pos;
                    while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t' ||
                           body[pos] == '\r' || body[pos] == '\n'))
                        ++pos;
                    if (pos >= body.size() || (body[pos] != '"' && body[pos] != 39))
                        break;

                    const char value_quote = body[pos++];
                    const unsigned int value_start = pos;
                    while (pos < body.size() && body[pos] != value_quote)
                        ++pos;
                    if (pos >= body.size())
                        break;
                    tag->attributes[key] = body.substr(value_start, pos - value_start);
                    ++pos;
                }
                return true;
            }
            return false;
        }

        bool getAttribute(const XmlTag& tag, const char* name, std::string* value)
        {
            std::map<std::string, std::string>::const_iterator it =
                tag.attributes.find(name);
            if (it == tag.attributes.end())
                return false;
            *value = it->second;
            return true;
        }

        bool parseUnsigned(const std::string& text, unsigned int* value)
        {
            if (text.empty() || !value)
                return false;
            char* end = 0;
            const unsigned long parsed = std::strtoul(text.c_str(), &end, 10);
            if (!end || *end != '\0' || parsed > 0xFFFFFFFFul)
                return false;
            *value = static_cast<unsigned int>(parsed);
            return true;
        }

        bool parseFloat(const std::string& text, float* value)
        {
            if (!value)
                return false;
            std::istringstream input(text);
            input >> *value;
            return !input.fail();
        }

        bool parseBoolean(const std::string& text)
        {
            return text == "true" || text == "1" || text == "Y" ||
                   text == "yes" || text == "TRUE";
        }

        bool parsePoint(const std::string& text,
                        const std::vector<TrackQuad>& quads,
                        TrackPoint* point)
        {
            const std::string::size_type colon = text.find(':');
            if (colon != std::string::npos)
            {
                unsigned int quad_index;
                unsigned int point_index;
                if (!parseUnsigned(text.substr(0, colon), &quad_index) ||
                    !parseUnsigned(text.substr(colon + 1), &point_index) ||
                    quad_index >= quads.size() || point_index >= 4)
                    return false;
                *point = quads[quad_index].points[point_index];
                return true;
            }

            std::istringstream input(text);
            return static_cast<bool>(input >> point->x >> point->y >> point->z);
        }

        void addEdge(TrackData* data, unsigned int from, unsigned int to)
        {
            if (from >= data->successors.size() || to >= data->successors.size())
                return;
            std::vector<unsigned int>& edges = data->successors[from];
            for (unsigned int i = 0; i < edges.size(); ++i)
            {
                if (edges[i] == to)
                    return;
            }
            edges.push_back(to);
        }

        void finishQuad(TrackQuad* quad)
        {
            quad->center.x = 0.0f;
            quad->center.y = 0.0f;
            quad->center.z = 0.0f;
            quad->min_y = quad->points[0].y;
            quad->max_y = quad->points[0].y;
            for (unsigned int i = 0; i < 4; ++i)
            {
                quad->center.x += quad->points[i].x * 0.25f;
                quad->center.y += quad->points[i].y * 0.25f;
                quad->center.z += quad->points[i].z * 0.25f;
                if (quad->points[i].y < quad->min_y)
                    quad->min_y = quad->points[i].y;
                if (quad->points[i].y > quad->max_y)
                    quad->max_y = quad->points[i].y;
            }
        }
    }

    TrackPoint::TrackPoint() : x(0.0f), y(0.0f), z(0.0f)
    {
    }

    TrackQuad::TrackQuad()
        : min_y(0.0f), max_y(0.0f), invisible(false), ai_ignore(false)
    {
    }

    TrackData::TrackData()
    {
        clear();
    }

    void TrackData::clear()
    {
        name.clear();
        default_laps = 3;
        reverse_available = false;
        min_height_testing = -1.0f;
        max_height_testing = 5.0f;
        main_loop_start = 0;
        main_loop_end = 0;
        quads.clear();
        successors.clear();
    }

    bool TrackData::containsPoint(unsigned int quad_index,
                                  float x, float y, float z) const
    {
        if (quad_index >= quads.size())
            return false;

        const TrackQuad& quad = quads[quad_index];
        if (y - quad.max_y > max_height_testing ||
            y - quad.min_y < min_height_testing)
            return false;

        bool inside = false;
        for (unsigned int i = 0, j = 3; i < 4; j = i++)
        {
            const float xi = quad.points[i].x;
            const float zi = quad.points[i].z;
            const float xj = quad.points[j].x;
            const float zj = quad.points[j].z;
            const bool crosses = ((zi > z) != (zj > z)) &&
                (x < (xj - xi) * (z - zi) / ((zj - zi) == 0.0f ? 0.000001f : (zj - zi)) + xi);
            if (crosses)
                inside = !inside;
        }
        return inside;
    }

    bool TrackData::findQuad(float x, float y, float z,
                             unsigned int hint, unsigned int* result) const
    {
        if (!result || quads.empty())
            return false;
        if (hint < quads.size() && containsPoint(hint, x, y, z))
        {
            *result = hint;
            return true;
        }
        for (unsigned int i = 0; i < quads.size(); ++i)
        {
            if (i != hint && containsPoint(i, x, y, z))
            {
                *result = i;
                return true;
            }
        }
        return false;
    }

    unsigned int TrackData::firstSuccessor(unsigned int quad_index) const
    {
        if (quad_index >= successors.size() || successors[quad_index].empty())
            return quad_index;
        return successors[quad_index][0];
    }

    TrackReadResult TrackDataReader::readQuads(const char* xml,
                                                unsigned int size,
                                                TrackData* output,
                                                std::string* error_message)
    {
        if (!xml || !output)
        {
            setError(error_message, "null quads XML or output");
            return TRACK_READ_INVALID_ARGUMENT;
        }

        output->quads.clear();
        output->successors.clear();
        output->min_height_testing = -1.0f;
        output->max_height_testing = 5.0f;

        unsigned int cursor = 0;
        XmlTag tag;
        bool root_found = false;
        while (parseTag(xml, size, &cursor, &tag))
        {
            if (tag.closing)
                continue;
            if (tag.name == "quads")
            {
                root_found = true;
                continue;
            }
            if (tag.name == "height-testing")
            {
                std::string value;
                if (getAttribute(tag, "min", &value) &&
                    !parseFloat(value, &output->min_height_testing))
                {
                    setError(error_message, "invalid quads height-testing minimum");
                    return TRACK_READ_INVALID_DATA;
                }
                if (getAttribute(tag, "max", &value) &&
                    !parseFloat(value, &output->max_height_testing))
                {
                    setError(error_message, "invalid quads height-testing maximum");
                    return TRACK_READ_INVALID_DATA;
                }
                continue;
            }
            if (tag.name != "quad")
                continue;
            if (output->quads.size() >= kMaxQuads)
            {
                setError(error_message, "track has too many drive quads");
                return TRACK_READ_LIMIT_EXCEEDED;
            }

            TrackQuad quad;
            const char* names[4] = { "p0", "p1", "p2", "p3" };
            for (unsigned int i = 0; i < 4; ++i)
            {
                std::string value;
                if (!getAttribute(tag, names[i], &value) ||
                    !parsePoint(value, output->quads, &quad.points[i]))
                {
                    setError(error_message, "invalid or unresolved track quad point");
                    return TRACK_READ_INVALID_DATA;
                }
            }

            std::string value;
            if (getAttribute(tag, "invisible", &value))
                quad.invisible = parseBoolean(value);
            if (getAttribute(tag, "ai-ignore", &value))
                quad.ai_ignore = parseBoolean(value);

            finishQuad(&quad);
            output->quads.push_back(quad);
        }

        if (!root_found || output->quads.empty())
        {
            setError(error_message, "quads XML contains no usable quads root/data");
            return TRACK_READ_BAD_XML;
        }

        output->main_loop_start = 0;
        output->main_loop_end = static_cast<unsigned int>(output->quads.size() - 1);
        if (error_message)
            error_message->clear();
        return TRACK_READ_OK;
    }

    TrackReadResult TrackDataReader::readGraph(const char* xml,
                                                unsigned int size,
                                                TrackData* output,
                                                std::string* error_message)
    {
        if (!xml || !output || output->quads.empty())
        {
            setError(error_message, "graph XML requires parsed quads");
            return TRACK_READ_INVALID_ARGUMENT;
        }

        output->successors.clear();
        output->successors.resize(output->quads.size());

        unsigned int cursor = 0;
        XmlTag tag;
        bool root_found = false;
        unsigned int total_edges = 0;
        bool loop_found = false;

        while (parseTag(xml, size, &cursor, &tag))
        {
            if (tag.closing)
                continue;
            if (tag.name == "graph")
            {
                root_found = true;
                continue;
            }

            unsigned int from = 0;
            unsigned int to = 0;
            if (tag.name == "node-list")
            {
                std::string value;
                if (getAttribute(tag, "from-quad", &value) &&
                    (!parseUnsigned(value, &from) || from >= output->quads.size()))
                {
                    setError(error_message, "invalid graph node-list start");
                    return TRACK_READ_INVALID_DATA;
                }
                if (getAttribute(tag, "to-quad", &value) &&
                    (!parseUnsigned(value, &to) || to >= output->quads.size()))
                {
                    setError(error_message, "invalid graph node-list end");
                    return TRACK_READ_INVALID_DATA;
                }
                continue;
            }
            if (tag.name != "edge-loop" && tag.name != "edge-line" &&
                tag.name != "edge")
                continue;

            std::string value;
            if (!getAttribute(tag, "from", &value) || !parseUnsigned(value, &from) ||
                !getAttribute(tag, "to", &value) || !parseUnsigned(value, &to) ||
                from >= output->quads.size() || to >= output->quads.size())
            {
                setError(error_message, "invalid graph edge endpoints");
                return TRACK_READ_INVALID_DATA;
            }

            if (tag.name == "edge-loop")
            {
                if (from > to)
                {
                    setError(error_message, "invalid graph edge-loop range");
                    return TRACK_READ_INVALID_DATA;
                }
                if (!loop_found)
                {
                    output->main_loop_start = from;
                    output->main_loop_end = to;
                    loop_found = true;
                }
                for (unsigned int i = from; i <= to; ++i)
                {
                    addEdge(output, i, i == to ? from : i + 1);
                    if (++total_edges > kMaxEdges)
                    {
                        setError(error_message, "track graph has too many edges");
                        return TRACK_READ_LIMIT_EXCEEDED;
                    }
                }
            }
            else if (tag.name == "edge-line")
            {
                if (from > to)
                {
                    setError(error_message, "invalid graph edge-line range");
                    return TRACK_READ_INVALID_DATA;
                }
                for (unsigned int i = from; i < to; ++i)
                {
                    addEdge(output, i, i + 1);
                    if (++total_edges > kMaxEdges)
                    {
                        setError(error_message, "track graph has too many edges");
                        return TRACK_READ_LIMIT_EXCEEDED;
                    }
                }
            }
            else
            {
                addEdge(output, from, to);
                if (++total_edges > kMaxEdges)
                {
                    setError(error_message, "track graph has too many edges");
                    return TRACK_READ_LIMIT_EXCEEDED;
                }
            }
        }

        if (!root_found)
        {
            setError(error_message, "invalid graph XML root");
            return TRACK_READ_BAD_XML;
        }

        if (!loop_found)
        {
            output->main_loop_start = 0;
            output->main_loop_end = static_cast<unsigned int>(output->quads.size() - 1);
        }

        for (unsigned int i = 0; i < output->successors.size(); ++i)
        {
            if (output->successors[i].empty())
                addEdge(output, i, i + 1 < output->quads.size() ? i + 1 :
                        output->main_loop_start);
        }

        if (error_message)
            error_message->clear();
        return TRACK_READ_OK;
    }

    TrackReadResult TrackDataReader::readTrackInfo(const char* xml,
                                                    unsigned int size,
                                                    TrackData* output,
                                                    std::string* error_message)
    {
        if (!xml || !output)
        {
            setError(error_message, "null track XML or output");
            return TRACK_READ_INVALID_ARGUMENT;
        }

        unsigned int cursor = 0;
        XmlTag tag;
        while (parseTag(xml, size, &cursor, &tag))
        {
            if (tag.closing || tag.name != "track")
                continue;

            std::string value;
            if (getAttribute(tag, "name", &value))
                output->name = value;
            if (getAttribute(tag, "default-number-of-laps", &value) &&
                !parseUnsigned(value, &output->default_laps))
            {
                setError(error_message, "invalid default track lap count");
                return TRACK_READ_INVALID_DATA;
            }
            if (getAttribute(tag, "reverse", &value))
                output->reverse_available = parseBoolean(value);
            if (error_message)
                error_message->clear();
            return TRACK_READ_OK;
        }

        setError(error_message, "track XML has no track root element");
        return TRACK_READ_BAD_XML;
    }
}
