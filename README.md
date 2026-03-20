# WODO

![](./assets/wodo.png)

_Wodo_ is a vim-based task management system.

<img width="1289" height="842" alt="image" src="https://github.com/user-attachments/assets/a1ec1ffc-6963-4435-a1fa-f447c83f7e6e" />

> [!WARNING]
> **It's currently in development, it may have some bugs**

## Dependencies

- [Openssl Lib for hases](https://www.openssl.org/)
- [Clang +14](https://pt.wikipedia.org/wiki/Clang)

## Building + Installing + Nvim configuring + Uninstalling

**To build the project you just need run one command:**

```bash
make BUILD=1
```

This will create a native executable inside `bin` folder.

**To install it, you can just run:**

```bash
sudo make install # requires sudo if you want to use the default prefix /usr/local/bin
```

You can set a prefix where the `wodo` binary goes by using the env `PREFIX`.
By default, the prefix is going to be `/usr/local/bin`.

**Uninstalling:**

```bash
sudo make uninstall # requires sudo if you want to use the default prefix /usr/local/bin
```

> [!WARNING]
> **It will update your neovim configs**
> This nvim configuration is based on [my own neovim config](https://github.com/marcos-venicius/config-manager).

So, if you don't have the same, you can configure it manually.

By installing using `make install`, those operations are going to be done on your neovim configurations:

```bash
ln -sf $(PWD)/nvim/ftdetect/wodo.lua $(USER_HOME)/.config/nvim/ftdetect/wodo.lua
ln -sf $(PWD)/nvim/syntax/wodo.vim $(USER_HOME)/.config/nvim/syntax/wodo.vim
ln -sf $(PWD)/nvim/lua/config/wodo.lua $(USER_HOME)/.config/nvim/lua/config/wodo.lua
```

> [!NOTE]
> I'm using Snacks plugin default notifier for notifications and telescope for pickers

## Formatting files with `<leader>wf`

https://github.com/user-attachments/assets/fb920f90-5fe5-4c13-aebd-a85c8be5d13d
