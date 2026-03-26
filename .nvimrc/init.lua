local dap = require("dap")

vim.keymap.set("n", "<M-m>", function()
	vim.cmd("Compile bin/build.sh")
end, {})

local dap_c_config = {
	{
		name = "Debug Game (codelldb)",
		type = "codelldb",
		request = "launch",
		program = vim.fn.getcwd() .. "/target/inkshot",
		cwd = vim.fn.getcwd() .. "/target",
		stopOnEntry = false,
	},
}
dap.configurations.c = dap_c_config
dap.configurations.cpp = dap_c_config
