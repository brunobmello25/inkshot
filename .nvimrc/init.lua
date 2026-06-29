local dap = require("dap")

vim.keymap.set("n", "<M-m>", function()
	vim.cmd("Compile bin/build.sh")
end, {})

local dap_c_config = {
	{
		name = "Debug Game (codelldb)",
		type = "codelldb",
		request = "launch",
		program = vim.fn.getcwd() .. "/data/inkshot",
		cwd = vim.fn.getcwd() .. "/data",
		stopOnEntry = false,
	},
}

dap.configurations.c = dap_c_config
dap.configurations.cpp = dap_c_config

dap.configurations.odin = {
	{
		name = "Debug (codelldb)",
		type = "codelldb",
		request = "launch",
		program = vim.fn.getcwd() .. "/data/inkshot",
		cwd = vim.fn.getcwd() .. "/data",
		stopOnEntry = false,
	},
}
