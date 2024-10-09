## mycssparse


A tiny tool to parse simple css file.

2024-10-08


Cascadia Code 为命令行和代码编辑器提供了全新的体验，并且它还支持编程连字（Programming Ligatures），就是在编写代码的时候，可以组合字符创建新的字形，这对于代码的可读性和呈现后的用户友好度都是一种很好的机制，同时这也增强了 Windows Terminal 的现代外观。

VS 内置环境变量:

	$(SolutionDir) 	    解决方案目录：即.sln文件所在路径
	$(ProjectDir) 	    项目根目录:, 即.vcxproj文件所在路径
	$(Configuration)    当前的编译配置名称，比如Debug，或Release
	$(ProjectName) 	    当前项目名称
	$(SolutionName)     解决方案名称
	$(OutDir)           项目输出文件目录
	$(TargetDir)        项目输出文件目录
	$(TargetName)       项目生成目标文件, 通常和$(ProjectName)同名, 如Game
	$(TargetExt)        项目生成文件后缀名，如.exe, .lib具体取决于工程设置
	$(TargetFileName)   项目输出文件名字。比如Game.exe, 等于 (TargetName)+(TargetExt)
	$(ProjectExt)       工程文件后缀名，如.vcxproj
