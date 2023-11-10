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

std::shared_ptr<type> type::deserialize(state &state, std::istream& in) {
    char type;
    in >> type;

    switch (type) {
        case 'p':
            return type_primitive::deserialize(state, in);
        case 'u':
            return type_undefined::deserialize(state, in);
        case 'v':
            return type_void::deserialize(state, in);
        case 'f':
            return type_function::deserialize(state, in);
        case 'n':
            return type_ns::deserialize(state, in);
        case 'c':
            return type_custom::deserialize(state, in);
        case 'o':
            return type_object::deserialize(state, in);
        default:
            parser::report_message(parser::report_type::error, "Could not deserialize type", std::cerr);
            exit(1);
    }
}

std::shared_ptr<type> type_undefined::deserialize(state &state, std::istream& in) {
    return std::make_shared<type_undefined>();
}

std::shared_ptr<type> type_void::deserialize(state &state, std::istream& in) {
    return std::make_shared<type_void>();
}

void type_primitive::serialize(std::ostream& out) const {
    type::serialize(out);
    out << get_fqn();
    out << '\0';
}

std::shared_ptr<type> type_primitive::deserialize(state &state, std::istream& in) {
    return type::create_builtin(read_string(in));
}

void type_ns::serialize(std::ostream& out) const {
    type::serialize(out);
    out << name << '\0';
}

std::shared_ptr<type> type_ns::deserialize(state &state, std::istream& in) {
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

std::shared_ptr<type> type_function::deserialize(state &state, std::istream& in) {
    auto return_type = type::deserialize(state, in);
    auto parameters = read_vector<std::shared_ptr<type>>(in, [&state](auto& in) {
        return type::deserialize(state, in);
    });
    auto fn = type::create_function(return_type, parameters);
    auto fn_t = std::static_pointer_cast<type_function>(fn);

    if (read_boolean(in)) {
        fn_t->method_of = deserialize_object_type_reference<type_custom>(state, in);
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

static member deserialize_member(state &state, std::istream& in) {
    std::string name = serializable::ISerializable::read_string(in);
    auto type = type::deserialize(state, in);
    std::unordered_set<ast::decl_classifier> classifiers;
    serializable::ISerializable::read_unordered_set<ast::decl_classifier>(
        in, classifiers, [](auto& in) {
        return string_to_classifier(serializable::ISerializable::read_string(in));
    });

    return member(name, type, nullptr, classifiers);
}

void type_custom::serialize(std::ostream& out) const {
    type::serialize(out);

    if (catalyst::isa<const type_struct>(this)) {
        out << "s";
    } else if (catalyst::isa<const type_virtual>(this)) {
        out << "v";
    } else {
        parser::report_message(parser::report_type::error, "Could not serialize custom type", std::cerr);
        exit(1);
    }

    out << name << '\0';
    out << std::string(init_function->getName()) << '\0';
    write_vector<member>(out, members, [](auto& out, const auto& member) {
        serialize_member(member, out);
    });
}

std::shared_ptr<type> type_custom::deserialize(state &state, std::istream& in) {
    char stype;
    in >> stype;

    std::string name = serializable::ISerializable::read_string(in);
    std::string init_function_name = serializable::ISerializable::read_string(in);
    auto members = serializable::ISerializable::read_vector<member>(in, [&state](auto& in) {
        return deserialize_member(state, in);
    });

    if (stype == 's') {
        return type_struct::deserialize(state, in, name, init_function_name, members);
    } else if (stype == 'v') {
        return type_virtual::deserialize(state, in, name, init_function_name, members);
    } else {
        parser::report_message(parser::report_type::error, "Could not deserialize custom type", std::cerr);
        exit(1);
    }
}

void type_struct::serialize(std::ostream& out) const {
    type_custom::serialize(out);
}

std::shared_ptr<type> type_struct::deserialize(state &state, std::istream& in, const std::string &name, const std::string &init_function_name, const std::vector<member> &members) {
    auto struct_ = std::make_shared<type_struct>(name, members);
    //struct_->init_function = state.module->getFunction(init_function_name); // TODO: make sure we reference the correct function
    return struct_;

}

void type_virtual::serialize(std::ostream& out) const {
    type_custom::serialize(out);

    if (catalyst::isa<const type_class>(this)) {
        out << "c";
    } else {
        parser::report_message(parser::report_type::error, "Could not serialize virtual type", std::cerr);
        exit(1);
    }

    write_vector<object_type_reference<type_virtual>>(out, this->super, [](auto& out, const auto& super) {
        serialize_object_type_reference(super, out);
    });
}

std::shared_ptr<type> type_virtual::deserialize(state &state, std::istream& in, const std::string &name, const std::string &init_function_name, const std::vector<member> &members) {
    char vtype;
    in >> vtype;

    auto super = read_vector<object_type_reference<type_virtual>>(in, [&state](auto& in) {
        return deserialize_object_type_reference<type_virtual>(state, in);
    });

    if (vtype == 'c') {
        return type_class::deserialize(state, in, name, init_function_name, super, members);
    } else {
        parser::report_message(parser::report_type::error, "Could not deserialize virtual type", std::cerr);
        exit(1);
    }
}

void type_class::serialize(std::ostream& out) const {
    type_virtual::serialize(out);
}

std::shared_ptr<type> type_class::deserialize(state &state, std::istream& in, const std::string &name, const std::string &init_function_name, const std::vector<object_type_reference<type_virtual>> &super, const std::vector<member> &members) {
    auto class_ = std::make_shared<type_class>(name, super, members);
    //class_->init_function = state.module->getFunction(init_function_name); // TODO: make sure we reference the correct function
    return class_;
}

void type_object::serialize(std::ostream& out) const {
    type::serialize(out);
    serialize_object_type_reference(this->object_type, out);
}

std::shared_ptr<type> type_object::deserialize(state &state, std::istream& in) {
    return std::make_shared<type_object>(deserialize_object_type_reference<type_custom>(state, in));
}

} // namespace catalyst::compiler::codegen