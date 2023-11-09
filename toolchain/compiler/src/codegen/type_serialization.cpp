#include "catalyst/rtti.hpp"

#include "type.hpp"
#include "decl_classifiers.hpp"

namespace catalyst::compiler::codegen {

void type::serialize(std::ostream& out) const {
    if (catalyst::isa<const type_primitive>(this)) {
        out << "p";
    } else if (catalyst::isa<const type_undefined>(this)) {
        out << "u";
    } else if (catalyst::isa<const type_void>(this)) {
        out << "v";
    } else if (catalyst::isa<const type_function>(this)) {
        out << "f";
    } else if (catalyst::isa<const type_ns>(this)) {
        out << "n";
    } else if (catalyst::isa<const type_custom>(this)) {
        out << "c";
    } else if (catalyst::isa<const type_object>(this)) {
        out << "o";
    } else {
        parser::report_message(parser::report_type::error, "Could not serialize type", std::cerr);
        exit(1);
    }
}

std::shared_ptr<type> type::deserialize(std::istream& in) {
    return nullptr;
}

std::shared_ptr<type> type_undefined::deserialize(std::istream& in) {
    return std::make_shared<type_undefined>();
}

std::shared_ptr<type> type_void::deserialize(std::istream& in) {
    return std::make_shared<type_void>();
}

void type_primitive::serialize(std::ostream& out) const {
    type::serialize(out);
    out << get_fqn();
    out << '\0';
}

std::shared_ptr<type> type_primitive::deserialize(std::istream& in) {
    return type::create_builtin(read_string(in));
}

void type_ns::serialize(std::ostream& out) const {
    type::serialize(out);
    out << name;
}

std::shared_ptr<type> type_ns::deserialize(std::istream& in) {
    return std::make_shared<type_ns>(read_string(in));
}

template <typename T>
void serialize_object_type_reference(const object_type_reference<T> &ref, std::ostream& out) {
    out << ref.getName() << '\0';
}

template <typename T>
object_type_reference<T> deserialize_object_type_reference(state &state, std::istream& in) {
    std::string name = serializable::ISerializable::read_string(in);
    return object_type_reference<T>(state, name);
}

void type_function::serialize(std::ostream& out) const {
    type::serialize(out);
    return_type->serialize(out);
    write_vector<std::shared_ptr<type>>(out, parameters, [](auto& out, const auto& param) {
        param->serialize(out);
    });
    
    if (method_of.has_value()) {
        out << '\1';
        serialize_object_type_reference<type_custom>(method_of.value(), out);
    } else {
        out << '\0';
    }
}

std::shared_ptr<type> type_function::deserialize(std::istream& in) {
    auto return_type = type::deserialize(in);
    auto parameters = read_vector<std::shared_ptr<type>>(in, [](auto& in) {
        return type::deserialize(in);
    });
    auto fn = type::create_function(return_type, parameters);
    auto fn_t = std::static_pointer_cast<type_function>(fn);

    if (read_boolean(in)) {
        //TODO: fn_t->method_of = deserialize_object_type_reference(, in);
    }

    return fn;
}

static void serialize_member(const member &member, std::ostream& out) {
    out << member.name << '\0';
    member.type->serialize(out);
    serializable::ISerializable::write_unordered_set<ast::decl_classifier>(
        out, member.classifiers, [](auto& out, const auto& classifier) {
        out << classifier_to_string(classifier) << '\0';
    });
}

void type_custom::serialize(std::ostream& out) const {
    type::serialize(out);
    out << name << '\0';
    out << std::string(init_function->getName()) << '\0';
    write_vector<member>(out, members, [](auto& out, const auto& member) {
        serialize_member(member, out);
    });
}

void type_struct::serialize(std::ostream& out) const {
    type_custom::serialize(out);
}

void type_virtual::serialize(std::ostream& out) const {
    type_custom::serialize(out);

    write_vector<object_type_reference<type_virtual>>(out, this->super, [](auto& out, const auto& super) {
        serialize_object_type_reference(super, out);
    });
}



void type_class::serialize(std::ostream& out) const {
    type_virtual::serialize(out);
}

void type_object::serialize(std::ostream& out) const {
    type::serialize(out);
    serialize_object_type_reference(this->object_type, out);
}

} // namespace catalyst::compiler::codegen