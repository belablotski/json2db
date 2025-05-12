# Notes about using GitHub Copilot

The role of an engineer is changing - insted of writing the code, you're guiding an agent and more read/review the outcome. Also, you're a safeguard since Agent generages some shell/Python scripts, so you need to review them before allowing it to run them.

Also, with the grow of the project it becomes more and more difficult to explain what exactly changes you want to make. ANd wice-versa, it becomes more and more difficult to Copilot to actually make a clear-cut decisions.

## "Ask" mode

1. Great for pointed questions with narror context. For example, when you don't remember API format. I.e. in "ask" mode you work more like bottom-up - from the lines of code to a final result.

## "Agent" mode

1. Great for project-wide changes and implementation of bigger components. In agent mode you work more like top-down, asking Agent to make some changes and then drilling down - reviewing them and fixing when necessary.
2. Agent is capable of establishing a feedback loop for themselves. For example, when it sees an error of a missing library, it tries to install it and re-do whatever it executed before.
3. Agent is capable to adjust VSCode configuration (to configure build for example).
4. Agent is capable of installing software (it installed Postgres in my Linux machine and helped me to connect to the server from terminal).
5. Agent is capable of understanding the context, and generate new code in a way how the existing code is written. For example, initially it proposed me to use private fields with the underscore at the end, but I prefer to have them started from underscore. So, after changing it a couple of first times, the Agent started to propose to the code with in _privateVariable format.
