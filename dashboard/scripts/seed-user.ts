/**
 * Seed a user into Firestore for NextAuth credentials login.
 *
 * Usage:
 *   npx ts-node scripts/seed-user.ts <username> <password>
 *
 * Requires GOOGLE_APPLICATION_CREDENTIALS or GCP_PROJECT_ID env var.
 */
import { Firestore } from '@google-cloud/firestore';
import bcrypt from 'bcryptjs';

async function main() {
  const [username, password] = process.argv.slice(2);

  if (!username || !password) {
    console.error('Usage: npx ts-node scripts/seed-user.ts <username> <password>');
    process.exit(1);
  }

  const firestore = new Firestore({
    projectId: process.env.GCP_PROJECT_ID,
  });

  const passwordHash = await bcrypt.hash(password, 12);

  await firestore.collection('users').doc(username).set({
    name: username,
    passwordHash,
    createdAt: new Date(),
  });

  console.log(`User "${username}" created in Firestore.`);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
