#include "catalyst/rtti.hpp"

#include "type.hpp"

namespace catalyst::compiler::codegen {

void type::serialize(std::ostream& out) const {
    if (catalyst::isa<const type_primitive>(this)) {
        out << "p";
        auto* this_p = static_cast<const type_primitive*>(this);
        out << this_p->fqn;
    } else if (catalyst::isa<const type_undefined>(this)) {
        out << "u";
    } else if (catalyst::isa<const type_void>(this)) {
        out << "v";
    } else if (catalyst::isa<const type_function>(this)) {
        out << "f";
    } else if (catalyst::isa<const type_ns>(this)) {
        out << "n";
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

void type_function::serialize(std::ostream& out) const {
    type::serialize(out);
    return_type->serialize(out);
    write_vector<std::shared_ptr<type>>(out, parameters, [](auto& out, const auto& param) {
        param->serialize(out);
    });
    
    if (method_of != nullptr) {
        out << true;
        method_of->serialize(out);
    } else {
        out << false;
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
        fn_t->method_of = std::static_pointer_cast<type_custom>(type::deserialize(in));
    }

    return fn;
}

void type_custom::serialize(std::ostream& out) const {
    type::serialize(out);
    out << name;
}

} // namespace catalyst::compiler::codegen