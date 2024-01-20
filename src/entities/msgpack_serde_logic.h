#pragma once

#include <vector>
#include <string>
#include <msgpack.hpp>

#include "common.h"

// Serialization and deserialization logic for the primary column type.
namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
    namespace adaptor {

    using net_zelcon::wikidice::entities::CategoryLinkRecord;
    using net_zelcon::wikidice::entities::CategoryWeight;

    // CategoryWeight

    template <> struct convert<CategoryWeight> {
        msgpack::object const &
        operator()(const msgpack::object &o,
                   CategoryWeight &v) const {
            if (o.type != msgpack::type::ARRAY)
                throw msgpack::type_error();
            if (o.via.array.size != 2)
                throw msgpack::type_error();
            if (o.via.array.ptr[0].type != msgpack::type::POSITIVE_INTEGER)
                throw msgpack::type_error();
            v.depth = o.via.array.ptr[0].as<uint8_t>();
            if (o.via.array.ptr[1].type != msgpack::type::POSITIVE_INTEGER)
                throw msgpack::type_error();
            v.weight = o.via.array.ptr[1].as<uint64_t>();
            return o;
        }
    };

    template <> struct pack<CategoryWeight> {
        template <typename Stream>
        packer<Stream> &
        operator()(msgpack::packer<Stream> &o,
                   CategoryWeight const &v) const {
            o.pack_array(2);
            o.pack(v.depth);
            o.pack(v.weight);
            return o;
        }
    };

    template <> struct object_with_zone<CategoryWeight> {
        void operator()(msgpack::object::with_zone& o, CategoryWeight const& v) const {
            o.type = type::ARRAY;
            o.via.array.size = 2;
            o.via.array.ptr = static_cast<msgpack::object*>(o.zone.allocate_align(sizeof(msgpack::object) * o.via.array.size, MSGPACK_ZONE_ALIGNOF(msgpack::object)));
            o.via.array.ptr[0] = msgpack::object{v.depth, o.zone};
            o.via.array.ptr[1] = msgpack::object{v.weight, o.zone};
        }
    };


    // CategoryLinkRecord

    template <> struct convert<CategoryLinkRecord> {
        msgpack::object const &
        operator()(const msgpack::object &o,
                   CategoryLinkRecord &v) const {
            if (o.type != msgpack::type::ARRAY)
                throw msgpack::type_error();
            if (o.via.array.size != 3)
                throw msgpack::type_error();
            v.pages_mut() = o.via.array.ptr[0].as<std::vector<uint64_t>>();
            v.subcategories_mut() = o.via.array.ptr[1].as<std::vector<uint64_t>>();
            v.weights_mut() = o.via.array.ptr[2].as<std::vector<CategoryWeight>>();
            return o;
        }
    };

    template <> struct pack<CategoryLinkRecord> {
        template <typename Stream>
        packer<Stream> &
        operator()(msgpack::packer<Stream> &o,
                   CategoryLinkRecord const &v) const {
            o.pack_array(3);
            o.pack(v.pages());
            o.pack(v.subcategories());
            o.pack(v.weights());
            return o;
        }
    };

    template <>
    struct object_with_zone<CategoryLinkRecord> {
        void
        operator()(msgpack::object::with_zone &o,
                   CategoryLinkRecord const &v) const {
            o.type = type::ARRAY;
            o.via.array.size = 3;
            o.via.array.ptr =
                static_cast<msgpack::object *>(o.zone.allocate_align(
                    sizeof(msgpack::object) * o.via.array.size,
                    MSGPACK_ZONE_ALIGNOF(msgpack::object)));
            o.via.array.ptr[0] = msgpack::object{v.pages(), o.zone};
            o.via.array.ptr[1] = msgpack::object{v.subcategories(), o.zone};
            o.via.array.ptr[2] = msgpack::object{v.weights(), o.zone};
        }
    };

    } // namespace adaptor
} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // namespace msgpack
