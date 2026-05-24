from setuptools import setup

setup(
    name='example',
    version='21.12',
    license='Apache Software License',
    packages=['example'],
    entry_points={
        'console_scripts': ['example = example:main', 'Example=example:main'],
    },
)
