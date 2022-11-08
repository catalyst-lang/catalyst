import os
import re

script_path = os.path.dirname(os.path.realpath(__file__))

with open (script_path + '/src/grammar.hpp', 'r' ) as f:
    content = f.read()
    content = re.sub('#include [^\n]*\n', '', content, flags = re.M)
    content = re.sub('#pragma [^\n]*\n', '', content, flags = re.M)
    #content = re.sub('static constexpr auto value\\s*=\\s*lexy::callback((\\((\\([^)]*\\)|[^)])*\\))|.|\\n)+?(?=\\);)\\);', '//value', content, flags = re.M)
    #content = re.sub('static constexpr auto value\\s*=\\s*lexy::callback(.+?(?=\\);\\s*};)\\);\\s*};)', '//value\n};', content, flags = re.M)
    #content = re.sub('static constexpr auto value\\s*=\\s*lexy::callback(.+?(?=}\\);)}\\);)', '//value', content, flags = re.M)
    content = re.sub('static constexpr auto value\\s*=[^;]+;', '//value', content, flags = re.M)
    content = re.sub('static constexpr auto value\\s*=', '//value', content, flags = re.M)
    #content = re.sub('\\/\\/value((.|\\n)+?(?=};))^};', '};', content, flags = re.M)
    content = re.sub('<ast::numeric_classifier>', '<int>', content, flags = re.M)
    content = re.sub('(ast::numeric_classifier::[^)]+)', '0', content, flags = re.M)
    content = re.sub('<ast::[^>]+>', '', content, flags = re.M)
    print(content)
