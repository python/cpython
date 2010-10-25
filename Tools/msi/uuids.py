# This should be extended for each Python release.
# The product code must change whenever the name of the MSI file
# changes, and when new component codes are issued for existing
# components. See "Changing the Product Code". As we change the
# component codes with every build, we need a new product code
# each time. For intermediate (snapshot) releases, they are automatically
# generated. For official releases, we record the product codes,
# so people can refer to them.
product_codes = {
    '2.5.101': '{bc14ce3e-5e72-4a64-ac1f-bf59a571898c}', # 2.5a1
    '2.5.102': '{5eed51c1-8e9d-4071-94c5-b40de5d49ba5}', # 2.5a2
    '2.5.103': '{73dcd966-ffec-415f-bb39-8342c1f47017}', # 2.5a3
    '2.5.111': '{c797ecf8-a8e6-4fec-bb99-526b65f28626}', # 2.5b1
    '2.5.112': '{32beb774-f625-439d-b587-7187487baf15}', # 2.5b2
    '2.5.113': '{89f23918-11cf-4f08-be13-b9b2e6463fd9}', # 2.5b3
    '2.5.121': '{8e9321bc-6b24-48a3-8fd4-c95f8e531e5f}', # 2.5c1
    '2.5.122': '{a6cd508d-9599-45da-a441-cbffa9f7e070}', # 2.5c2
    '2.5.150': '{0a2c5854-557e-48c8-835a-3b9f074bdcaa}', # 2.5.0
    '2.5.1121':'{0378b43e-6184-4c2f-be1a-4a367781cd54}', # 2.5.1c1
    '2.5.1150':'{31800004-6386-4999-a519-518f2d78d8f0}', # 2.5.1
    '2.5.2150':'{6304a7da-1132-4e91-a343-a296269eab8a}', # 2.5.2c1
    '2.5.2150':'{6b976adf-8ae8-434e-b282-a06c7f624d2f}', # 2.5.2
    '2.6.101': '{0ba82e1b-52fd-4e03-8610-a6c76238e8a8}', # 2.6a1
    '2.6.102': '{3b27e16c-56db-4570-a2d3-e9a26180c60b}', # 2.6a2
    '2.6.103': '{cd06a9c5-bde5-4bd7-9874-48933997122a}', # 2.6a3
    '2.6.104': '{dc6ed634-474a-4a50-a547-8de4b7491e53}', # 2.6a4
    '2.6.111': '{3f82079a-5bee-4c4a-8a41-8292389e24ae}', # 2.6b1
    '2.6.112': '{8a0e5970-f3e6-4737-9a2b-bc5ff0f15fb5}', # 2.6b2
    '2.6.113': '{df4f5c21-6fcc-4540-95de-85feba634e76}', # 2.6b3
    '2.6.121': '{bbd34464-ddeb-4028-99e5-f16c4a8fbdb3}', # 2.6c1
    '2.6.122': '{8f64787e-a023-4c60-bfee-25d3a3f592c6}', # 2.6c2
    '2.6.150': '{110eb5c4-e995-4cfb-ab80-a5f315bea9e8}', # 2.6.0
    '2.6.1150':'{9cc89170-000b-457d-91f1-53691f85b223}', # 2.6.1
    '2.6.2121':'{adac412b-b209-4c15-b6ab-dca1b6e47144}', # 2.6.2c1
    '2.6.2150':'{24aab420-4e30-4496-9739-3e216f3de6ae}', # 2.6.2
    '2.6.3121':'{a73e0254-dcda-4fe4-bf37-c7e1c4f4ebb6}', # 2.6.3c1
    '2.6.3150':'{3d9ac095-e115-4e94-bdef-7f7edf17697d}', # 2.6.3
    '2.6.4121':'{727de605-0359-4606-a94b-c2033652379b}', # 2.6.4c1
    '2.6.4122':'{4f7603c6-6352-4299-a398-150a31b19acc}', # 2.6.4c2
    '2.6.4150':'{e7394a0f-3f80-45b1-87fc-abcd51893246}', # 2.6.4
    '2.6.5121':'{e0e273d7-7598-4701-8325-c90c069fd5ff}', # 2.6.5c1
    '2.6.5122':'{fa227b76-0671-4dc6-b826-c2ff2a70dfd5}', # 2.6.5c2
    '2.6.5150':'{4723f199-fa64-4233-8e6e-9fccc95a18ee}', # 2.6.5
    '2.7.101': '{eca1bbef-432c-49ae-a667-c213cc7bbf22}', # 2.7a1
    '2.7.102': '{21ce16ed-73c4-460d-9b11-522f417b2090}', # 2.7a2
    '2.7.103': '{6e7dbd55-ba4a-48ac-a688-6c75db4d7500}', # 2.7a3
    '2.7.104': '{ee774ba3-74a5-48d9-b425-b35a287260c8}', # 2.7a4
    '2.7.111': '{9cfd9ec7-a9c7-4980-a1c6-054fc6493eb3}', # 2.7b1
    '2.7.112': '{9a72faf6-c304-4165-8595-9291ff30cac6}', # 2.7b2
    '2.7.121': '{f530c94a-dd53-4de9-948e-b632b9cb48d2}', # 2.7c1
    '2.7.122': '{f80905d2-dd8d-4b8e-8a40-c23c93dca07d}', # 2.7c2
    '2.7.150': '{20c31435-2a0a-4580-be8b-ac06fc243ca4}', # 2.7.0
    '2.7.1121':'{60a4036a-374c-4fd2-84b9-bfae7db03931}', # 2.7.1rc1
    '2.7.1122':'{5965e7d1-5584-4de9-b13a-694e0b2ee3a6}', # 2.7.1rc2
    '2.7.1150':'{32939827-d8e5-470a-b126-870db3c69fdf}', # 2.7.1
}
