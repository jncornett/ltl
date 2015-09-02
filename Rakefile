require "erb"

CXX_FLAGS = "-Wall -Wextra -pedantic -Wformat -Wformat-security -Wno-deprecated-declarations"
CMAKE_ARGS = [
  "-DCMAKE_CXX_FLAGS=#{CXX_FLAGS.shellescape}"
]

LOG_PATH = File.expand_path("log")

def task_name(t)
  t.name.split(":").last
end

class Template
  def initialize(path)
    @path = path
  end

  def to_s
    File.open(@path) { |file| file.read }
  end

  def render(binding)
    ERB.new(to_s).result(binding)
  end

  def create(binding, out_path)
    File.open(out_path, "w") { |file| file.write(render(binding)) }
  end

  def self.generate(binding, res_path, out_path)
    self.new(res_path).create(binding, out_path)
  end
end

directory "build"

task :cmake => ["build"] do
  Dir.chdir("build") do
    sh "cmake #{CMAKE_ARGS.join(" ")} .. >/dev/null"
  end
end

namespace :make do
  task :all => [:cmake] do
    Dir.chdir("build") { sh "make -j`sysctl -n hw.ncpu` >/dev/null" }
  end

  task :demo => [:cmake] do
    Dir.chdir("build") { sh "make -j`sysctl -n hw.ncpu` demo >/dev/null" }
  end

  task :tests => [:cmake] do
    Dir.chdir("build") { sh "make -j`sysctl -n hw.ncpu` tests >/dev/null" }
  end
end

task :make => ["make:all"]

task :demo => ["make:demo"] do
  sh "build/demo/demo"
end

task :tests => ["make:tests"] do
  sh "build/tests/tests"
end

task :clean do
  rm_rf "build"
end

namespace :add do
  def do_template(task, name)
    fail "no name specified" unless name

    name.gsub!(/\.h$/, name)
    dst_path = "src/#{name}.h"
    res_path = "rakelib/#{task}.template"

    fail unless File.exist?(res_path)
    if File.exist?(dst_path)
      warn "file #{dst_path} already exists. nothing to do"
      return
    end

    # setup binding
    norma_liz = name.gsub(/[^\w\. \/_-]/, 'x').split(/[\. \/_-]+/)
    snake_name = norma_liz.join("_").downcase
    const_name = snake_name.upcase
    camel_name = norma_liz.collect { |x| x.capitalize }.join('')

    # do templating
    erb = ERB.new(File.open(res_path).read)
    File.open(dst_path, "w").write(erb.result(binding))
  end

  task :header, [:name] do |t, args|
    do_template task_name(t), args[:name]
  end

  task :test, [:name] do |t, args|
    do_template task_name(t), args[:name]
  end

  task :module, [:name] => ["add:header", "add:test"]
end

task :default => [:make, :tests]
