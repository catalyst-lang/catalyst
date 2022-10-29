import platform
env = Environment(SCONS_CXX_STANDARD="c++17")
env['SYSTEM'] = platform.system().lower()

if env['SYSTEM'] in ['linux', 'darwin']:
    env.Append( CCFLAGS=["-std=c++17"] )
if env['SYSTEM'] == 'windows':
    env.Append( CCFLAGS=["/std:c++20", "/EHsc"] )

def set_debug():
    if env['SYSTEM'] in ['linux', 'darwin']:
        env.Append( CCFLAGS=["-g"] )
    if env['SYSTEM'] == 'windows':
        env.Append( CCFLAGS=["/Zi", "/DEBUG", "/D_DEBUG", "/MDd"] )
        env.Append( LINKFLAGS=["/DEBUG"])

def set_release():
    if env['SYSTEM'] in ['linux', 'darwin']:
        env.Append(CCFLAGS=[])
    if env['SYSTEM'] == 'windows':
        env.Append(CCFLAGS=["/MD"] )
        env.Append(LINKFLAGS=[])

if ARGUMENTS.get('DEBUG') == '1':
    set_debug()
else:
    set_release()

#Progress(['-\r', '\\\r', '|\r', '/\r'], interval=5)

env.Program(target='hello', source='hello.cpp')

Export('env')
SConscript('toolchain/SConscript')
