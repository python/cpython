{% for section in sections %}
{% set underline = "-" %}
{% if sections[section] %}
{% if section %}
{{section}}
{{ underline * section|length }}{% set underline = "~" %}


{% endif %}
{% for category, val in definitions.items() if category in sections[section]%}
{% if sections[section][category] %}
{% if sections[section]|length > 1 %}
{{ definitions[category]['name'] }}
{{ underline * definitions[category]['name']|length }}

{% endif %}
{% if definitions[category]['showcontent'] %}
{% for text, values in sections[section][category]|dictsort(by='value') %}
- {{ text }} ({{ values|sort|join(', ') }})
{% endfor %}
{% else %}
- {{ sections[section][category]['']|sort|join(', ') }}
{% endif %}
{% endif %}

{% endfor %}

{% endif %}
{% endfor %}
