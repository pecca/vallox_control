import { NextAuthOptions } from 'next-auth';
import CredentialsProvider from 'next-auth/providers/credentials';
import bcrypt from 'bcryptjs';
import { getFirestore } from './firestore';

export const authOptions: NextAuthOptions = {
  providers: [
    CredentialsProvider({
      name: 'Credentials',
      credentials: {
        username: { label: 'Username', type: 'text' },
        password: { label: 'Password', type: 'password' },
      },
      async authorize(credentials) {
        if (!credentials?.username || !credentials?.password) {
          return null;
        }

        const db = getFirestore();
        const userDoc = await db.collection('users').doc(credentials.username).get();

        if (!userDoc.exists) {
          return null;
        }

        const userData = userDoc.data()!;
        const isValid = await bcrypt.compare(credentials.password, userData.passwordHash);

        if (!isValid) {
          return null;
        }

        return {
          id: userDoc.id,
          name: userData.name || userDoc.id,
        };
      },
    }),
  ],
  session: {
    strategy: 'jwt',
    maxAge: 24 * 60 * 60,
  },
  pages: {
    signIn: '/login',
  },
};
