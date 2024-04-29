import tiktoken
import subprocess

text = input("Text to complete: ")
enc = tiktoken.get_encoding("gpt2")

tokens = [
    str(tok) for tok in enc.encode(text)
]

proc = subprocess.Popen(
    ["./gpt-64", *tokens],
    stdout=subprocess.PIPE,
    text=True
)

while (line := proc.stdout.readline()):
    token = int(line)
    print(enc.decode([token]), end='', flush=True) # Print the token，这里实际上是每个单词对应一个编码，然后通过token转变为单词
