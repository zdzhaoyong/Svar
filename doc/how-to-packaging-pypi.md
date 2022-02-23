## Setup your pypi account

```
gedit ~/.pypirc
```

```
[distutils]
index-servers =
  pypi
  testpypi

[pypi]
repository=https://pypi.python.org/pypi
username=your_username
password=your_password

[testpypi]
repository=https://testpypi.python.org/pypi
username=your_username
password=your_password
```

## Packaging to source

```
python3 setup.py sdist 
```

## Upload the package to pypi

```
python3 setup.py sdist upload -r pypi
```
